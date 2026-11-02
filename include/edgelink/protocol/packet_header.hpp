#pragma once

#include <cstdint>
#include <cstddef>
#include <endian.h>

namespace edgelink::protocol {

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t magic{0};
    uint8_t  version{0};
    uint8_t  flags{0};
    uint32_t device_id{0};
    uint64_t timestamp_ms{0};
    uint32_t payload_length{0};
};
#pragma pack(pop)

static_assert(sizeof(PacketHeader) == 20, "PacketHeader must be exactly 20 bytes");

inline PacketHeader to_network_order(PacketHeader hdr) noexcept {
    hdr.magic = htobe16(hdr.magic);
    hdr.device_id = htobe32(hdr.device_id);
    hdr.timestamp_ms = htobe64(hdr.timestamp_ms);
    hdr.payload_length = htobe32(hdr.payload_length);
    return hdr;
}

inline PacketHeader to_host_order(PacketHeader hdr) noexcept {
    hdr.magic = be16toh(hdr.magic);
    hdr.device_id = be32toh(hdr.device_id);
    hdr.timestamp_ms = be64toh(hdr.timestamp_ms);
    hdr.payload_length = be32toh(hdr.payload_length);
    return hdr;
}

}
