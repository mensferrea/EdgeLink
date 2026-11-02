#include "edgelink/network/udp_server.hpp"
#include "edgelink/protocol/protocol_parser.hpp"
#include "edgelink/service/metrics_registry.hpp"
#include "edgelink/common/types.hpp"
#include <unistd.h>
#include <cerrno>
#include <array>

namespace edgelink::network {

UdpServer::UdpServer(uint16_t port, EpollEventLoop& event_loop, PacketDispatchCallback dispatcher)
    : event_loop_(event_loop),
      dispatcher_(std::move(dispatcher)),
      socket_(Socket::create_udp()) {
    socket_.set_reuse_address(true);
    socket_.set_reuse_port(true);
    socket_.bind(port);
    socket_.set_nonblocking(true);

    event_loop_.add_fd(socket_.native_handle(), EPOLLIN | EPOLLET);
}

UdpServer::~UdpServer() {
    if (socket_.is_valid()) {
        try {
            event_loop_.remove_fd(socket_.native_handle());
        } catch (...) {}
    }
}

int UdpServer::socket_fd() const noexcept {
    return socket_.native_handle();
}

void UdpServer::handle_event(int fd, uint32_t events) {
    if (fd == socket_.native_handle() && (events & EPOLLIN)) {
        read_datagrams();
    }
}

void UdpServer::read_datagrams() {
    std::array<uint8_t, MAX_PACKET_PAYLOAD_SIZE> buffer;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        ssize_t n = ::recvfrom(
            socket_.native_handle(),
            buffer.data(),
            buffer.size(),
            0,
            reinterpret_cast<sockaddr*>(&client_addr),
            &addr_len
        );

        if (n > 0) {
            service::MetricsRegistry::instance().record_udp_datagram(static_cast<size_t>(n));

            auto result = protocol::ProtocolParser::parse_packet(
                std::span<const uint8_t>(buffer.data(), static_cast<size_t>(n))
            );

            if (result.status == protocol::ParseStatus::Success && result.record.has_value()) {
                service::MetricsRegistry::instance().record_packet_parsed();
                if (dispatcher_) {
                    dispatcher_(std::move(*result.record));
                }
            } else {
                service::MetricsRegistry::instance().record_packet_corrupted();
            }
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            break;
        }
    }
}

}
