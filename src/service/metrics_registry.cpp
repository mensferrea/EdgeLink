#include "edgelink/service/metrics_registry.hpp"
#include <sstream>

namespace edgelink::service {

std::string MetricsSnapshot::to_json() const {
    std::ostringstream ss;
    ss << "{"
       << "\"tcp_connections_accepted\":" << tcp_connections_accepted << ","
       << "\"tcp_connections_closed\":" << tcp_connections_closed << ","
       << "\"active_tcp_connections\":" << active_tcp_connections << ","
       << "\"tcp_bytes_received\":" << tcp_bytes_received << ","
       << "\"udp_datagrams_received\":" << udp_datagrams_received << ","
       << "\"udp_bytes_received\":" << udp_bytes_received << ","
       << "\"packets_parsed_total\":" << packets_parsed_total << ","
       << "\"packets_corrupted_total\":" << packets_corrupted_total << ","
       << "\"packets_out_of_range\":" << packets_out_of_range << ","
       << "\"queue_dropped_total\":" << queue_dropped_total << ","
       << "\"packets_processed_total\":" << packets_processed_total
       << "}";
    return ss.str();
}

MetricsRegistry& MetricsRegistry::instance() noexcept {
    static MetricsRegistry reg;
    return reg;
}

void MetricsRegistry::record_tcp_accept() noexcept {
    ++tcp_connections_accepted_;
    ++active_tcp_connections_;
}

void MetricsRegistry::record_tcp_close() noexcept {
    ++tcp_connections_closed_;
    --active_tcp_connections_;
}

void MetricsRegistry::record_tcp_bytes(size_t bytes) noexcept {
    tcp_bytes_received_ += bytes;
}

void MetricsRegistry::record_udp_datagram(size_t bytes) noexcept {
    ++udp_datagrams_received_;
    udp_bytes_received_ += bytes;
}

void MetricsRegistry::record_packet_parsed() noexcept {
    ++packets_parsed_total_;
}

void MetricsRegistry::record_packet_corrupted() noexcept {
    ++packets_corrupted_total_;
}

void MetricsRegistry::record_packet_out_of_range() noexcept {
    ++packets_out_of_range_;
}

void MetricsRegistry::record_queue_dropped() noexcept {
    ++queue_dropped_total_;
}

void MetricsRegistry::record_packet_processed() noexcept {
    ++packets_processed_total_;
}

MetricsSnapshot MetricsRegistry::snapshot() const noexcept {
    return MetricsSnapshot{
        .tcp_connections_accepted = tcp_connections_accepted_.load(std::memory_order_relaxed),
        .tcp_connections_closed = tcp_connections_closed_.load(std::memory_order_relaxed),
        .active_tcp_connections = active_tcp_connections_.load(std::memory_order_relaxed),
        .tcp_bytes_received = tcp_bytes_received_.load(std::memory_order_relaxed),
        .udp_datagrams_received = udp_datagrams_received_.load(std::memory_order_relaxed),
        .udp_bytes_received = udp_bytes_received_.load(std::memory_order_relaxed),
        .packets_parsed_total = packets_parsed_total_.load(std::memory_order_relaxed),
        .packets_corrupted_total = packets_corrupted_total_.load(std::memory_order_relaxed),
        .packets_out_of_range = packets_out_of_range_.load(std::memory_order_relaxed),
        .queue_dropped_total = queue_dropped_total_.load(std::memory_order_relaxed),
        .packets_processed_total = packets_processed_total_.load(std::memory_order_relaxed)
    };
}

void MetricsRegistry::reset() noexcept {
    tcp_connections_accepted_.store(0, std::memory_order_relaxed);
    tcp_connections_closed_.store(0, std::memory_order_relaxed);
    active_tcp_connections_.store(0, std::memory_order_relaxed);
    tcp_bytes_received_.store(0, std::memory_order_relaxed);
    udp_datagrams_received_.store(0, std::memory_order_relaxed);
    udp_bytes_received_.store(0, std::memory_order_relaxed);
    packets_parsed_total_.store(0, std::memory_order_relaxed);
    packets_corrupted_total_.store(0, std::memory_order_relaxed);
    packets_out_of_range_.store(0, std::memory_order_relaxed);
    queue_dropped_total_.store(0, std::memory_order_relaxed);
    packets_processed_total_.store(0, std::memory_order_relaxed);
}

}
