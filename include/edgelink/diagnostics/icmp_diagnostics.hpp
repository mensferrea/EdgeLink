#pragma once

#include "edgelink/network/socket.hpp"
#include <string>
#include <cstdint>
#include <optional>

namespace edgelink::diagnostics {

struct IcmpPingResult {
    bool success{false};
    std::string target_ip;
    double rtt_ms{0.0};
    uint16_t sequence{0};
    uint8_t ttl{0};
    std::string error_message;
};

class IcmpDiagnostics {
public:
    IcmpDiagnostics();
    ~IcmpDiagnostics() = default;

    [[nodiscard]] bool is_available() const noexcept;
    [[nodiscard]] const std::string& init_error() const noexcept;

    [[nodiscard]] IcmpPingResult ping(const std::string& target_ip, uint32_t timeout_ms = 1000, uint16_t seq = 1);

private:
    static uint16_t calculate_checksum(const void* data, size_t length) noexcept;

    std::optional<network::Socket> socket_;
    std::string init_error_;
    uint16_t process_id_{0};
};

}
