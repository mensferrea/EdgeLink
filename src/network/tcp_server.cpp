#include "edgelink/network/tcp_server.hpp"
#include "edgelink/service/metrics_registry.hpp"
#include <unistd.h>
#include <cerrno>

namespace edgelink::network {

TcpServer::TcpServer(uint16_t port, EpollEventLoop& event_loop, PacketDispatchCallback dispatcher)
    : event_loop_(event_loop),
      dispatcher_(std::move(dispatcher)),
      listen_socket_(Socket::create_tcp()) {
    listen_socket_.set_reuse_address(true);
    listen_socket_.set_reuse_port(true);
    listen_socket_.bind(port);
    listen_socket_.set_nonblocking(true);
    listen_socket_.listen();

    event_loop_.add_fd(listen_socket_.native_handle(), EPOLLIN | EPOLLET);
}

TcpServer::~TcpServer() {
    close_all_connections();
    if (listen_socket_.is_valid()) {
        try {
            event_loop_.remove_fd(listen_socket_.native_handle());
        } catch (...) {}
    }
}

int TcpServer::listen_fd() const noexcept {
    return listen_socket_.native_handle();
}

size_t TcpServer::connection_count() const noexcept {
    return connections_.size();
}

void TcpServer::handle_event(int fd, uint32_t events) {
    if (fd == listen_socket_.native_handle()) {
        if (events & EPOLLIN) {
            accept_clients();
        }
    } else {
        if (events & EPOLLIN) {
            process_client(fd);
        }

        if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
            close_client(fd);
        }
    }
}

void TcpServer::accept_clients() {
    while (true) {
        std::string client_ip;
        uint16_t client_port = 0;
        try {
            Socket client_sock = listen_socket_.accept(client_ip, client_port);
            int fd = client_sock.native_handle();
            auto conn = std::make_unique<Connection>(std::move(client_sock), std::move(client_ip), client_port);

            event_loop_.add_fd(fd, EPOLLIN | EPOLLET | EPOLLRDHUP);
            connections_.emplace(fd, std::move(conn));
            service::MetricsRegistry::instance().record_tcp_accept();
        } catch (const std::system_error& e) {
            if (e.code().value() == EAGAIN || e.code().value() == EWOULDBLOCK) {
                break;
            }
            break;
        }
    }
}

void TcpServer::process_client(int client_fd) {
    auto it = connections_.find(client_fd);
    if (it == connections_.end()) {
        return;
    }

    std::vector<model::TelemetryRecord> records;
    size_t bytes_read = 0;
    size_t corrupted_count = 0;

    ConnectionState state = it->second->read_incoming(records, bytes_read, corrupted_count);

    if (bytes_read > 0) {
        service::MetricsRegistry::instance().record_tcp_bytes(bytes_read);
    }
    if (corrupted_count > 0) {
        for (size_t i = 0; i < corrupted_count; ++i) {
            service::MetricsRegistry::instance().record_packet_corrupted();
        }
    }

    for (auto& record : records) {
        service::MetricsRegistry::instance().record_packet_parsed();
        if (dispatcher_) {
            dispatcher_(std::move(record));
        }
    }

    if (state == ConnectionState::Closed || state == ConnectionState::Error) {
        close_client(client_fd);
    }
}

void TcpServer::close_client(int client_fd) {
    auto it = connections_.find(client_fd);
    if (it != connections_.end()) {
        try {
            event_loop_.remove_fd(client_fd);
        } catch (...) {}
        connections_.erase(it);
        service::MetricsRegistry::instance().record_tcp_close();
    }
}

void TcpServer::close_all_connections() {
    for (auto& [fd, conn] : connections_) {
        try {
            event_loop_.remove_fd(fd);
        } catch (...) {}
        service::MetricsRegistry::instance().record_tcp_close();
    }
    connections_.clear();
}

}
