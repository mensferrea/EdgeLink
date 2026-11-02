#pragma once

#include "edgelink/model/tlv_record.hpp"
#include <cstdint>
#include <vector>
#include <optional>
#include <string>

namespace edgelink::model {

struct TelemetryRecord {
    uint32_t device_id{0};
    uint64_t timestamp_ms{0};
    std::vector<TlvRecord> metrics;

    [[nodiscard]] std::optional<double> get_metric(MetricType type) const noexcept;
    [[nodiscard]] bool has_metric(MetricType type) const noexcept;
    [[nodiscard]] std::string to_json() const;
};

}
