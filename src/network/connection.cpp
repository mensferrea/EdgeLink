#include "edgelink/network/connection.hpp"
#include "edgelink/protocol/protocol_parser.hpp"
#include "edgelink/common/types.hpp"
#include <unistd.h>
#include <cerrno>
#include <array>

namespace edgelink::network {

Connection::Connection(Socket socket, std::string peer_ip, uint16_t peer_port)
    : socket_(std::move(socket)),
      peer_ip_(std::move(peer_ip)),
      peer_port_(peer_port) {
    socket_.set_nonblocking(true);
}

int Connection::fd() const noexcept {
    return socket_.native_handle();
}

const std::string& Connection::peer_ip() const noexcept {
    return peer_ip_;
}

uint16_t Connection::peer_port() const noexcept {
    return peer_port_;
}

ConnectionState Connection::read_incoming(
    std::vector<model::TelemetryRecord>& output_records,
    size_t& bytes_read,
    size_t& corrupted_count
) {
    std::array<uint8_t, TCP_READ_BUFFER_SIZE> temp_buf;
    bytes_read = 0;
    corrupted_count = 0;

    while (true) {
        ssize_t n = ::read(socket_.native_handle(), temp_buf.data(), temp_buf.size());
        if (n > 0) {
            bytes_read += static_cast<size_t>(n);
            rx_buffer_.insert(rx_buffer_.end(), temp_buf.begin(), temp_buf.begin() + n);
        } else if (n == 0) {
            return ConnectionState::Closed;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == ECONNRESET) {
                return ConnectionState::Closed;
            }
            return ConnectionState::Error;
        }
    }

    if (!rx_buffer_.empty()) {
        auto parsed = protocol::ProtocolParser::parse_stream(rx_buffer_, corrupted_count);
        output_records.insert(
            output_records.end(),
            std::make_move_iterator(parsed.begin()),
            std::make_move_iterator(parsed.end())
        );
    }

    return ConnectionState::Active;
}

}
