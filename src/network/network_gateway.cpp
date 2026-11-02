#include "edgelink/network/network_gateway.hpp"
#include <array>
#include <unistd.h>

namespace edgelink::network {

NetworkGateway::NetworkGateway(GatewayConfig config, std::shared_ptr<service::DataProcessor> processor)
    : config_(config),
      processor_(std::move(processor)),
      queue_(config.queue_capacity) {
    if (config_.io_threads == 0) {
        config_.io_threads = 1;
    }
    if (config_.worker_threads == 0) {
        config_.worker_threads = 1;
    }
}

NetworkGateway::~NetworkGateway() {
    stop();
}

void NetworkGateway::start() {
    if (running_.exchange(true)) {
        return;
    }

    worker_threads_.reserve(config_.worker_threads);
    for (size_t i = 0; i < config_.worker_threads; ++i) {
        worker_threads_.emplace_back(&NetworkGateway::processing_worker, this);
    }

    io_threads_.reserve(config_.io_threads);
    for (size_t i = 0; i < config_.io_threads; ++i) {
        io_threads_.emplace_back(&NetworkGateway::io_loop_worker, this);
    }
}

void NetworkGateway::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    {
        std::lock_guard lock(event_loops_mutex_);
        for (auto& loop : event_loops_) {
            if (loop) {
                loop->stop();
            }
        }
    }

    for (auto& thread : io_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    io_threads_.clear();

    queue_.stop();

    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    worker_threads_.clear();

    {
        std::lock_guard lock(event_loops_mutex_);
        event_loops_.clear();
    }
}

bool NetworkGateway::is_running() const noexcept {
    return running_.load(std::memory_order_relaxed);
}

std::shared_ptr<service::DataProcessor> NetworkGateway::processor() const noexcept {
    return processor_;
}

const sync::ThreadSafeQueue<model::TelemetryRecord>& NetworkGateway::queue() const noexcept {
    return queue_;
}

void NetworkGateway::io_loop_worker() {
    auto loop = std::make_unique<EpollEventLoop>();
    EpollEventLoop* raw_loop = loop.get();

    {
        std::lock_guard lock(event_loops_mutex_);
        event_loops_.push_back(std::move(loop));
    }

    auto dispatcher = [this](model::TelemetryRecord record) {
        if (!queue_.try_push_drop_oldest(std::move(record))) {
            service::MetricsRegistry::instance().record_queue_dropped();
        }
    };

    TcpServer tcp_server(config_.tcp_port, *raw_loop, dispatcher);
    UdpServer udp_server(config_.udp_port, *raw_loop, dispatcher);

    std::array<epoll_event, edgelink::EPOLL_MAX_EVENTS> events{};

    while (running_.load(std::memory_order_relaxed) && raw_loop->is_running()) {
        int n = raw_loop->wait(events, 100);
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == raw_loop->wakeup_fd()) {
                uint64_t val = 0;
                ssize_t r = ::read(fd, &val, sizeof(val));
                (void)r;
                continue;
            }

            if (fd == udp_server.socket_fd()) {
                udp_server.handle_event(fd, ev);
            } else {
                tcp_server.handle_event(fd, ev);
            }
        }
    }
}

void NetworkGateway::processing_worker() {
    while (true) {
        model::TelemetryRecord record;
        if (!queue_.pop(record)) {
            break;
        }
        processor_->process(record);
    }
}

}
