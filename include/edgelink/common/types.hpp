#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

namespace edgelink {

inline constexpr uint16_t PROTOCOL_MAGIC = 0x544C;
inline constexpr uint8_t PROTOCOL_VERSION = 1;

inline constexpr uint16_t DEFAULT_TCP_PORT = 9001;
inline constexpr uint16_t DEFAULT_UDP_PORT = 9002;

inline constexpr size_t DEFAULT_QUEUE_CAPACITY = 20000;
inline constexpr size_t MAX_PACKET_PAYLOAD_SIZE = 65536;
inline constexpr size_t TCP_READ_BUFFER_SIZE = 8192;
inline constexpr size_t EPOLL_MAX_EVENTS = 128;

enum class TransportType : uint8_t {
    Tcp,
    Udp
};

}
