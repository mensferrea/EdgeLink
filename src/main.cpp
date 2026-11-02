#include "edgelink/network/network_gateway.hpp"
#include "edgelink/service/metrics_registry.hpp"
#include "edgelink/common/types.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <iomanip>

namespace {
std::atomic<bool> g_stop_requested{false};

void signal_handler(int) {
    g_stop_requested.store(true);
}
}

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    uint16_t tcp_port = edgelink::DEFAULT_TCP_PORT;
    uint16_t udp_port = edgelink::DEFAULT_UDP_PORT;
    size_t io_threads = 2;
    size_t worker_threads = 4;

    if (argc >= 3) {
        tcp_port = static_cast<uint16_t>(std::stoi(argv[1]));
        udp_port = static_cast<uint16_t>(std::stoi(argv[2]));
    }

    edgelink::network::GatewayConfig config{
        .tcp_port = tcp_port,
        .udp_port = udp_port,
        .io_threads = io_threads,
        .worker_threads = worker_threads,
        .queue_capacity = edgelink::DEFAULT_QUEUE_CAPACITY
    };

    auto processor = std::make_shared<edgelink::service::DataProcessor>();
    edgelink::network::NetworkGateway gateway(config, processor);

    std::cout << "Starting EdgeLink Gateway on TCP port " << tcp_port 
              << ", UDP port " << udp_port 
              << " (" << io_threads << " IO threads, " 
              << worker_threads << " worker threads)...\n";

    gateway.start();

    auto last_time = std::chrono::steady_clock::now();
    uint64_t last_processed = 0;

    while (!g_stop_requested.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - last_time).count();
        last_time = now;

        auto snapshot = edgelink::service::MetricsRegistry::instance().snapshot();
        uint64_t delta_processed = snapshot.packets_processed_total - last_processed;
        last_processed = snapshot.packets_processed_total;
        double throughput = elapsed_sec > 0 ? (static_cast<double>(delta_processed) / elapsed_sec) : 0.0;

        size_t active_devices = processor->aggregator()->get_active_device_count();

        std::cout << "[STATUS] Active Conns: " << snapshot.active_tcp_connections
                  << " | Devices: " << active_devices
                  << " | Parsed: " << snapshot.packets_parsed_total
                  << " | Corrupted: " << snapshot.packets_corrupted_total
                  << " | Out-of-Range: " << snapshot.packets_out_of_range
                  << " | Queue Dropped: " << snapshot.queue_dropped_total
                  << " | Processed: " << snapshot.packets_processed_total
                  << " (" << std::fixed << std::setprecision(1) << throughput << " msg/s)\n";
    }

    std::cout << "Shutting down EdgeLink Gateway...\n";
    gateway.stop();
    std::cout << "Gateway stopped successfully.\n";

    return 0;
}
