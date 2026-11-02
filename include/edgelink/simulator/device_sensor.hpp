#pragma once

#include "edgelink/model/telemetry_record.hpp"
#include <random>
#include <cstdint>

namespace edgelink::simulator {

class DeviceSensor {
public:
    explicit DeviceSensor(uint32_t device_id);

    [[nodiscard]] model::TelemetryRecord generate_record();

private:
    uint32_t device_id_;
    double phase_{0.0};
    std::mt19937 rng_;
    std::normal_distribution<double> noise_dist_{0.0, 0.5};
};

}
