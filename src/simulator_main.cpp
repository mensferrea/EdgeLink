#include "edgelink/simulator/device_simulator.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>

namespace {
std::atomic<bool> g_stop_requested{false};

void signal_handler(int) {
    g_stop_requested.store(true);
}
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    edgelink::simulator::SimulatorConfig config;
    config.device_count = 100;
    config.test_mode_corruptions = true;
    config.test_mode_partials = true;

    if (argc >= 2) {
        config.device_count = static_cast<size_t>(std::stoi(argv[1]));
    }
    if (argc >= 3) {
        config.target_ip = argv[2];
    }

    std::cout << "Starting Device Simulator with " << config.device_count 
              << " devices targeting " << config.target_ip 
              << " (TCP: " << config.tcp_port 
              << ", UDP: " << config.udp_port << ")...\n";

    edgelink::simulator::DeviceSimulator simulator(config);
    simulator.start();

    while (!g_stop_requested.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "[SIMULATOR] Total sent packets: " 
                  << simulator.total_sent_packets() << "\n";
    }

    std::cout << "Stopping Device Simulator...\n";
    simulator.stop();
    std::cout << "Simulator stopped.\n";

    return 0;
}
