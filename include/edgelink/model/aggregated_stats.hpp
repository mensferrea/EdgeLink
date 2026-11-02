#pragma once

#include "edgelink/model/metric_type.hpp"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <limits>
#include <sstream>
#include <iomanip>

namespace edgelink::model {

struct MetricSummary {
    double min{std::numeric_limits<double>::max()};
    double max{std::numeric_limits<double>::lowest()};
    double sum{0.0};
    double last_value{0.0};
    uint64_t count{0};

    void update(double val) noexcept {
        if (val < min) min = val;
        if (val > max) max = val;
        sum += val;
        last_value = val;
        ++count;
    }

    [[nodiscard]] double average() const noexcept {
        return count == 0 ? 0.0 : (sum / static_cast<double>(count));
    }
};

struct DeviceStatistics {
    uint32_t device_id{0};
    uint64_t total_packets{0};
    uint64_t last_update_ms{0};
    std::unordered_map<MetricType, MetricSummary> metrics;

    [[nodiscard]] std::string to_json() const {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "{\"device_id\":" << device_id
           << ",\"total_packets\":" << total_packets
           << ",\"last_update_ms\":" << last_update_ms
           << ",\"metrics\":{";

        bool first = true;
        for (const auto& [type, summary] : metrics) {
            if (!first) ss << ",";
            first = false;
            ss << "\"" << metric_type_to_string(type) << "\":{"
               << "\"min\":" << summary.min
               << ",\"max\":" << summary.max
               << ",\"avg\":" << summary.average()
               << ",\"last\":" << summary.last_value
               << ",\"count\":" << summary.count
               << "}";
        }
        ss << "}}";
        return ss.str();
    }
};

}
