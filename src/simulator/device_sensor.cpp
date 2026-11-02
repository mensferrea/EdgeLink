#include "edgelink/simulator/device_sensor.hpp"
#include <chrono>
#include <cmath>

namespace edgelink::simulator {

DeviceSensor::DeviceSensor(uint32_t device_id)
    : device_id_(device_id),
      rng_(device_id + 1337) {}

model::TelemetryRecord DeviceSensor::generate_record() {
    auto now = std::chrono::system_clock::now();
    uint64_t ts = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()
    );

    phase_ += 0.05;
    double sine = std::sin(phase_ + static_cast<double>(device_id_));

    double temp = 25.0 + 15.0 * sine + noise_dist_(rng_);
    double pressure = 10.0 + 3.0 * std::cos(phase_) + (noise_dist_(rng_) * 0.2);
    double voltage = 220.0 + 5.0 * sine + noise_dist_(rng_);
    double current = 15.0 + 2.0 * std::sin(phase_ * 0.5) + (noise_dist_(rng_) * 0.1);
    double vibration = 2.5 + 1.0 * std::abs(sine) + (noise_dist_(rng_) * 0.05);
    double humidity = 45.0 + 10.0 * std::cos(phase_ * 0.3) + noise_dist_(rng_);

    model::TelemetryRecord record;
    record.device_id = device_id_;
    record.timestamp_ms = ts;
    record.metrics.reserve(6);

    record.metrics.push_back({.type = model::MetricType::Temperature, .length = sizeof(double), .value = temp});
    record.metrics.push_back({.type = model::MetricType::Pressure,    .length = sizeof(double), .value = pressure});
    record.metrics.push_back({.type = model::MetricType::Voltage,     .length = sizeof(double), .value = voltage});
    record.metrics.push_back({.type = model::MetricType::Current,     .length = sizeof(double), .value = current});
    record.metrics.push_back({.type = model::MetricType::Vibration,   .length = sizeof(double), .value = vibration});
    record.metrics.push_back({.type = model::MetricType::Humidity,    .length = sizeof(double), .value = humidity});

    return record;
}

}
