#pragma once

#include "edgelink/model/telemetry_record.hpp"
#include "edgelink/service/telemetry_aggregator.hpp"
#include "edgelink/service/metrics_registry.hpp"
#include <functional>
#include <memory>

namespace edgelink::service {

struct MetricRange {
    double min_val;
    double max_val;
};

class DataProcessor {
public:
    using NormalizedOutputCallback = std::function<void(const model::TelemetryRecord&)>;

    explicit DataProcessor(std::shared_ptr<TelemetryAggregator> aggregator = std::make_shared<TelemetryAggregator>());

    [[nodiscard]] bool validate(const model::TelemetryRecord& record) const noexcept;
    [[nodiscard]] model::TelemetryRecord normalize(const model::TelemetryRecord& record) const;
    void process(const model::TelemetryRecord& record);

    void set_output_callback(NormalizedOutputCallback callback);
    [[nodiscard]] std::shared_ptr<TelemetryAggregator> aggregator() const noexcept;

private:
    std::shared_ptr<TelemetryAggregator> aggregator_;
    NormalizedOutputCallback output_callback_;
    mutable std::mutex callback_mutex_;
};

}
