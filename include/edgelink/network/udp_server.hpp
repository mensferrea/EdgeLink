#pragma once

#include "edgelink/network/socket.hpp"
#include "edgelink/network/epoll_event_loop.hpp"
#include "edgelink/model/telemetry_record.hpp"
#include <functional>

namespace edgelink::network {

class UdpServer {
public:
    using PacketDispatchCallback = std::function<void(model::TelemetryRecord)>;

    UdpServer(uint16_t port, EpollEventLoop& event_loop, PacketDispatchCallback dispatcher);
    ~UdpServer();

    UdpServer(const UdpServer&) = delete;
    UdpServer& operator=(const UdpServer&) = delete;

    void handle_event(int fd, uint32_t events);
    [[nodiscard]] int socket_fd() const noexcept;

private:
    void read_datagrams();

    EpollEventLoop& event_loop_;
    PacketDispatchCallback dispatcher_;
    Socket socket_;
};

}
