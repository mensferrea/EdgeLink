#pragma once

#include "edgelink/network/socket.hpp"
#include "edgelink/network/epoll_event_loop.hpp"
#include "edgelink/network/connection.hpp"
#include "edgelink/sync/thread_safe_queue.hpp"
#include <unordered_map>
#include <memory>
#include <functional>

namespace edgelink::network {

class TcpServer {
public:
    using PacketDispatchCallback = std::function<void(model::TelemetryRecord)>;

    TcpServer(uint16_t port, EpollEventLoop& event_loop, PacketDispatchCallback dispatcher);
    ~TcpServer();

    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    void handle_event(int fd, uint32_t events);
    [[nodiscard]] int listen_fd() const noexcept;
    [[nodiscard]] size_t connection_count() const noexcept;
    void close_all_connections();

private:
    void accept_clients();
    void process_client(int client_fd);
    void close_client(int client_fd);

    EpollEventLoop& event_loop_;
    PacketDispatchCallback dispatcher_;
    Socket listen_socket_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
};

}
