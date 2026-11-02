#include "edgelink/model/telemetry_record.hpp"
#include <sstream>
#include <iomanip>

namespace edgelink::model {

std::optional<double> TelemetryRecord::get_metric(MetricType type) const noexcept {
    for (const auto& metric : metrics) {
        if (metric.type == type) {
            return metric.value;
        }
    }
    return std::nullopt;
}

bool TelemetryRecord::has_metric(MetricType type) const noexcept {
    for (const auto& metric : metrics) {
        if (metric.type == type) {
            return true;
        }
    }
    return false;
}

std::string TelemetryRecord::to_json() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "{\"device_id\":" << device_id
       << ",\"timestamp_ms\":" << timestamp_ms
       << ",\"metrics\":{";

    for (size_t i = 0; i < metrics.size(); ++i) {
        if (i > 0) {
            ss << ",";
        }
        ss << "\"" << metric_type_to_string(metrics[i].type) << "\":" << metrics[i].value;
    }
    ss << "}}";
    return ss.str();
}

}
