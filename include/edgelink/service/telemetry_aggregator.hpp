#pragma once

#include "edgelink/model/telemetry_record.hpp"
#include "edgelink/model/aggregated_stats.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <vector>

namespace edgelink::service {

class TelemetryAggregator {
public:
    void update(const model::TelemetryRecord& record);
    [[nodiscard]] std::optional<model::DeviceStatistics> get_device_stats(uint32_t device_id) const;
    [[nodiscard]] std::vector<model::DeviceStatistics> get_all_stats() const;
    [[nodiscard]] size_t get_active_device_count() const;
    [[nodiscard]] std::string to_json() const;
    void reset();

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<uint32_t, model::DeviceStatistics> stats_map_;
};

}
