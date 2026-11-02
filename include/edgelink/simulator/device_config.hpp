#pragma once

#include "edgelink/common/types.hpp"
#include <string>
#include <chrono>

namespace edgelink::simulator {

struct DeviceConfig {
    uint32_t device_id{1};
    TransportType transport{TransportType::Tcp};
    std::string target_ip{"127.0.0.1"};
    uint16_t target_port{DEFAULT_TCP_PORT};
    std::chrono::milliseconds interval{100};
    bool introduce_corruptions{false};
    bool simulate_reconnections{false};
    bool simulate_partial_writes{false};
};

struct SimulatorConfig {
    size_t device_count{100};
    std::string target_ip{"127.0.0.1"};
    uint16_t tcp_port{DEFAULT_TCP_PORT};
    uint16_t udp_port{DEFAULT_UDP_PORT};
    std::chrono::milliseconds default_interval{100};
    double tcp_ratio{0.7};
    bool test_mode_corruptions{false};
    bool test_mode_partials{false};
};

}
