#include "edgelink/simulator/device_simulator.hpp"
#include "edgelink/protocol/protocol_parser.hpp"
#include "edgelink/network/socket.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <random>

namespace edgelink::simulator {

DeviceSimulator::DeviceSimulator(SimulatorConfig config)
    : config_(config) {}

DeviceSimulator::~DeviceSimulator() {
    stop();
}

void DeviceSimulator::start() {
    if (running_.exchange(true)) {
        return;
    }

    device_threads_.reserve(config_.device_count);
    size_t tcp_count = static_cast<size_t>(static_cast<double>(config_.device_count) * config_.tcp_ratio);

    for (size_t i = 1; i <= config_.device_count; ++i) {
        DeviceConfig dev_cfg{
            .device_id = static_cast<uint32_t>(i),
            .transport = (i <= tcp_count) ? TransportType::Tcp : TransportType::Udp,
            .target_ip = config_.target_ip,
            .target_port = (i <= tcp_count) ? config_.tcp_port : config_.udp_port,
            .interval = config_.default_interval,
            .introduce_corruptions = config_.test_mode_corruptions && (i % 7 == 0),
            .simulate_reconnections = (i % 5 == 0),
            .simulate_partial_writes = config_.test_mode_partials && (i % 3 == 0)
        };

        device_threads_.emplace_back(&DeviceSimulator::run_device_loop, this, dev_cfg);
    }
}

void DeviceSimulator::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    for (auto& t : device_threads_) {
        if (t.joinable()) {
            t.join();
        }
    }
    device_threads_.clear();
}

bool DeviceSimulator::is_running() const noexcept {
    return running_.load(std::memory_order_relaxed);
}

uint64_t DeviceSimulator::total_sent_packets() const noexcept {
    return total_sent_packets_.load(std::memory_order_relaxed);
}

void DeviceSimulator::run_device_loop(DeviceConfig dev_config) {
    DeviceSensor sensor(dev_config.device_id);
    std::mt19937 rng(dev_config.device_id);
    std::uniform_int_distribution<int> percent_dist(1, 100);

    std::string resolved_ip = network::Socket::resolve_host(dev_config.target_ip);

    network::Socket sock;
    bool connected = false;

    auto ensure_connected = [&]() -> bool {
        if (dev_config.transport == TransportType::Udp) {
            if (!sock.is_valid()) {
                try {
                    sock = network::Socket::create_udp();
                } catch (...) {
                    return false;
                }
            }
            return true;
        }

        if (connected && sock.is_valid()) {
            return true;
        }

        sock.close();
        try {
            sock = network::Socket::create_tcp();
            sock.connect(resolved_ip, dev_config.target_port);
            connected = true;
            return true;
        } catch (...) {
            connected = false;
            return false;
        }
    };

    uint64_t packet_counter = 0;

    while (running_.load(std::memory_order_relaxed)) {
        if (!ensure_connected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }

        auto record = sensor.generate_record();
        auto bytes = protocol::ProtocolParser::serialize(record);

        if (dev_config.introduce_corruptions && (percent_dist(rng) <= 15)) {
            if (bytes.size() > sizeof(protocol::PacketHeader)) {
                bytes[sizeof(protocol::PacketHeader) + 1] ^= 0xFF;
            }
        }

        if (dev_config.transport == TransportType::Tcp) {
            bool send_ok = true;
            if (dev_config.simulate_partial_writes && bytes.size() > 10) {
                size_t part1 = bytes.size() / 2;
                ssize_t w1 = ::write(sock.native_handle(), bytes.data(), part1);
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                ssize_t w2 = ::write(sock.native_handle(), bytes.data() + part1, bytes.size() - part1);
                send_ok = (w1 > 0 && w2 > 0);
            } else {
                ssize_t w = ::write(sock.native_handle(), bytes.data(), bytes.size());
                send_ok = (w > 0);
            }

            if (!send_ok) {
                connected = false;
                sock.close();
            } else {
                ++total_sent_packets_;
            }

            ++packet_counter;
            if (dev_config.simulate_reconnections && (packet_counter % 50 == 0)) {
                connected = false;
                sock.close();
            }
        } else {
            sockaddr_in dest{};
            dest.sin_family = AF_INET;
            dest.sin_port = htons(dev_config.target_port);
            ::inet_pton(AF_INET, resolved_ip.c_str(), &dest.sin_addr);

            ssize_t s = ::sendto(
                sock.native_handle(),
                bytes.data(),
                bytes.size(),
                0,
                reinterpret_cast<const sockaddr*>(&dest),
                sizeof(dest)
            );

            if (s > 0) {
                ++total_sent_packets_;
            }
        }

        std::this_thread::sleep_for(dev_config.interval);
    }
}

}
