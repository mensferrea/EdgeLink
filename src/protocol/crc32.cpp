#include "edgelink/protocol/crc32.hpp"
#include <array>

namespace edgelink::protocol {

namespace {

constexpr auto generate_crc_table() {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (int j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320u;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

constexpr auto CRC_TABLE = generate_crc_table();

}

uint32_t Crc32::calculate(std::span<const uint8_t> data) noexcept {
    return calculate(data.data(), data.size());
}

uint32_t Crc32::calculate(const void* data, size_t length) noexcept {
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < length; ++i) {
        uint8_t lookup_index = static_cast<uint8_t>((crc ^ bytes[i]) & 0xFF);
        crc = (crc >> 8) ^ CRC_TABLE[lookup_index];
    }
    return crc ^ 0xFFFFFFFFu;
}

}
