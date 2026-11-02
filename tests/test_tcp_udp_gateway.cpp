#include <gtest/gtest.h>
#include "edgelink/network/network_gateway.hpp"
#include "edgelink/network/socket.hpp"
#include "edgelink/protocol/protocol_parser.hpp"
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <atomic>

TEST(GatewayIntegrationTest, TcpAndUdpTransmission) {
    const uint16_t tcp_port = 19001;
    const uint16_t udp_port = 19002;

    edgelink::network::GatewayConfig config{
        .tcp_port = tcp_port,
        .udp_port = udp_port,
        .io_threads = 2,
        .worker_threads = 2,
        .queue_capacity = 1000
    };

    auto aggregator = std::make_shared<edgelink::service::TelemetryAggregator>();
    auto processor = std::make_shared<edgelink::service::DataProcessor>(aggregator);

    std::atomic<int> received_count{0};
    processor->set_output_callback([&received_count](const edgelink::model::TelemetryRecord&) {
        ++received_count;
    });

    edgelink::network::NetworkGateway gateway(config, processor);
    gateway.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    edgelink::model::TelemetryRecord tcp_rec{
        .device_id = 101,
        .timestamp_ms = 1000,
        .metrics = {{edgelink::model::MetricType::Temperature, 8, 24.5}}
    };
    auto tcp_bytes = edgelink::protocol::ProtocolParser::serialize(tcp_rec);

    auto client = edgelink::network::Socket::create_tcp();
    client.connect("127.0.0.1", tcp_port);
    ssize_t w = ::write(client.native_handle(), tcp_bytes.data(), tcp_bytes.size());
    EXPECT_EQ(w, static_cast<ssize_t>(tcp_bytes.size()));

    edgelink::model::TelemetryRecord udp_rec{
        .device_id = 102,
        .timestamp_ms = 2000,
        .metrics = {{edgelink::model::MetricType::Pressure, 8, 5.0}}
    };
    auto udp_bytes = edgelink::protocol::ProtocolParser::serialize(udp_rec);

    auto udp_sock = edgelink::network::Socket::create_udp();
    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(udp_port);
    ::inet_pton(AF_INET, "127.0.0.1", &dest.sin_addr);

    ssize_t s = ::sendto(
        udp_sock.native_handle(),
        udp_bytes.data(),
        udp_bytes.size(),
        0,
        reinterpret_cast<const sockaddr*>(&dest),
        sizeof(dest)
    );
    EXPECT_EQ(s, static_cast<ssize_t>(udp_bytes.size()));

    auto start_wait = std::chrono::steady_clock::now();
    while (received_count < 2 && std::chrono::steady_clock::now() - start_wait < std::chrono::seconds(2)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_GE(received_count.load(), 2);
    EXPECT_TRUE(aggregator->get_device_stats(101).has_value());
    EXPECT_TRUE(aggregator->get_device_stats(102).has_value());

    client.close();
    gateway.stop();
}
