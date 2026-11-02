#pragma once

#include <cstdint>
#include <cstddef>
#include <span>

namespace edgelink::protocol {

class Crc32 {
public:
    static uint32_t calculate(std::span<const uint8_t> data) noexcept;
    static uint32_t calculate(const void* data, size_t length) noexcept;
};

}
