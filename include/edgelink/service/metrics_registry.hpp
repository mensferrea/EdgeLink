#pragma once

#include <cstdint>
#include <atomic>
#include <string>

namespace edgelink::service {

struct MetricsSnapshot {
    uint64_t tcp_connections_accepted{0};
    uint64_t tcp_connections_closed{0};
    int64_t  active_tcp_connections{0};
    uint64_t tcp_bytes_received{0};
    uint64_t udp_datagrams_received{0};
    uint64_t udp_bytes_received{0};
    uint64_t packets_parsed_total{0};
    uint64_t packets_corrupted_total{0};
    uint64_t packets_out_of_range{0};
    uint64_t queue_dropped_total{0};
    uint64_t packets_processed_total{0};

    [[nodiscard]] std::string to_json() const;
};

class MetricsRegistry {
public:
    static MetricsRegistry& instance() noexcept;

    void record_tcp_accept() noexcept;
    void record_tcp_close() noexcept;
    void record_tcp_bytes(size_t bytes) noexcept;

    void record_udp_datagram(size_t bytes) noexcept;

    void record_packet_parsed() noexcept;
    void record_packet_corrupted() noexcept;
    void record_packet_out_of_range() noexcept;
    void record_queue_dropped() noexcept;
    void record_packet_processed() noexcept;

    [[nodiscard]] MetricsSnapshot snapshot() const noexcept;
    void reset() noexcept;

private:
    MetricsRegistry() = default;

    std::atomic<uint64_t> tcp_connections_accepted_{0};
    std::atomic<uint64_t> tcp_connections_closed_{0};
    std::atomic<int64_t>  active_tcp_connections_{0};
    std::atomic<uint64_t> tcp_bytes_received_{0};
    std::atomic<uint64_t> udp_datagrams_received_{0};
    std::atomic<uint64_t> udp_bytes_received_{0};
    std::atomic<uint64_t> packets_parsed_total_{0};
    std::atomic<uint64_t> packets_corrupted_total_{0};
    std::atomic<uint64_t> packets_out_of_range_{0};
    std::atomic<uint64_t> queue_dropped_total_{0};
    std::atomic<uint64_t> packets_processed_total_{0};
};

}
