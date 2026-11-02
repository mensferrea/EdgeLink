#include "edgelink/service/telemetry_aggregator.hpp"
#include <mutex>
#include <sstream>

namespace edgelink::service {

void TelemetryAggregator::update(const model::TelemetryRecord& record) {
    std::unique_lock lock(mutex_);
    auto& device_stats = stats_map_[record.device_id];
    device_stats.device_id = record.device_id;
    device_stats.last_update_ms = record.timestamp_ms;
    ++device_stats.total_packets;

    for (const auto& metric : record.metrics) {
        device_stats.metrics[metric.type].update(metric.value);
    }
}

std::optional<model::DeviceStatistics> TelemetryAggregator::get_device_stats(uint32_t device_id) const {
    std::shared_lock lock(mutex_);
    auto it = stats_map_.find(device_id);
    if (it != stats_map_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<model::DeviceStatistics> TelemetryAggregator::get_all_stats() const {
    std::shared_lock lock(mutex_);
    std::vector<model::DeviceStatistics> result;
    result.reserve(stats_map_.size());
    for (const auto& [_, stats] : stats_map_) {
        result.push_back(stats);
    }
    return result;
}

size_t TelemetryAggregator::get_active_device_count() const {
    std::shared_lock lock(mutex_);
    return stats_map_.size();
}

std::string TelemetryAggregator::to_json() const {
    std::shared_lock lock(mutex_);
    std::ostringstream ss;
    ss << "[";
    bool first = true;
    for (const auto& [_, stats] : stats_map_) {
        if (!first) ss << ",";
        first = false;
        ss << stats.to_json();
    }
    ss << "]";
    return ss.str();
}

void TelemetryAggregator::reset() {
    std::unique_lock lock(mutex_);
    stats_map_.clear();
}

}
