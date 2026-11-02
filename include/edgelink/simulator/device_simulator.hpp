#pragma once

#include "edgelink/simulator/device_config.hpp"
#include "edgelink/simulator/device_sensor.hpp"
#include <vector>
#include <thread>
#include <atomic>

namespace edgelink::simulator {

class DeviceSimulator {
public:
    explicit DeviceSimulator(SimulatorConfig config);
    ~DeviceSimulator();

    DeviceSimulator(const DeviceSimulator&) = delete;
    DeviceSimulator& operator=(const DeviceSimulator&) = delete;

    void start();
    void stop();
    [[nodiscard]] bool is_running() const noexcept;

    [[nodiscard]] uint64_t total_sent_packets() const noexcept;

private:
    void run_device_loop(DeviceConfig dev_config);

    SimulatorConfig config_;
    std::vector<std::thread> device_threads_;
    std::atomic<bool> running_{false};
    std::atomic<uint64_t> total_sent_packets_{0};
};

}
