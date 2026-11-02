#pragma once

#include "edgelink/common/types.hpp"
#include "edgelink/network/epoll_event_loop.hpp"
#include "edgelink/network/tcp_server.hpp"
#include "edgelink/network/udp_server.hpp"
#include "edgelink/sync/thread_safe_queue.hpp"
#include "edgelink/sync/thread_pool.hpp"
#include "edgelink/service/data_processor.hpp"
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

namespace edgelink::network {

struct GatewayConfig {
    uint16_t tcp_port{edgelink::DEFAULT_TCP_PORT};
    uint16_t udp_port{edgelink::DEFAULT_UDP_PORT};
    size_t io_threads{2};
    size_t worker_threads{4};
    size_t queue_capacity{edgelink::DEFAULT_QUEUE_CAPACITY};
};

class NetworkGateway {
public:
    explicit NetworkGateway(
        GatewayConfig config,
        std::shared_ptr<service::DataProcessor> processor = std::make_shared<service::DataProcessor>()
    );
    ~NetworkGateway();

    NetworkGateway(const NetworkGateway&) = delete;
    NetworkGateway& operator=(const NetworkGateway&) = delete;

    void start();
    void stop();
    [[nodiscard]] bool is_running() const noexcept;

    [[nodiscard]] std::shared_ptr<service::DataProcessor> processor() const noexcept;
    [[nodiscard]] const sync::ThreadSafeQueue<model::TelemetryRecord>& queue() const noexcept;

private:
    void io_loop_worker();
    void processing_worker();

    GatewayConfig config_;
    std::shared_ptr<service::DataProcessor> processor_;
    sync::ThreadSafeQueue<model::TelemetryRecord> queue_;

    std::vector<std::thread> io_threads_;
    std::vector<std::thread> worker_threads_;
    std::vector<std::unique_ptr<EpollEventLoop>> event_loops_;
    std::mutex event_loops_mutex_;

    std::atomic<bool> running_{false};
};

}
