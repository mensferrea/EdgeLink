#pragma once

#include "edgelink/network/socket.hpp"
#include "edgelink/model/telemetry_record.hpp"
#include <vector>
#include <string>

namespace edgelink::network {

enum class ConnectionState {
    Active,
    Closed,
    Error
};

class Connection {
public:
    Connection(Socket socket, std::string peer_ip, uint16_t peer_port);

    [[nodiscard]] int fd() const noexcept;
    [[nodiscard]] const std::string& peer_ip() const noexcept;
    [[nodiscard]] uint16_t peer_port() const noexcept;

    ConnectionState read_incoming(std::vector<model::TelemetryRecord>& output_records, size_t& bytes_read, size_t& corrupted_count);

private:
    Socket socket_;
    std::string peer_ip_;
    uint16_t peer_port_{0};
    std::vector<uint8_t> rx_buffer_;
};

}
