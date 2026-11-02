#pragma once

#include <cstdint>
#include <string_view>

namespace edgelink::model {

enum class MetricType : uint16_t {
    Temperature = 0x0001,
    Pressure    = 0x0002,
    Voltage     = 0x0003,
    Current     = 0x0004,
    Vibration   = 0x0005,
    Humidity    = 0x0006
};

constexpr std::string_view metric_type_to_string(MetricType type) noexcept {
    switch (type) {
        case MetricType::Temperature: return "Temperature";
        case MetricType::Pressure:    return "Pressure";
        case MetricType::Voltage:     return "Voltage";
        case MetricType::Current:     return "Current";
        case MetricType::Vibration:   return "Vibration";
        case MetricType::Humidity:    return "Humidity";
    }
    return "Unknown";
}

}
