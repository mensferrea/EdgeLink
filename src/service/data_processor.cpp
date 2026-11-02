#include "edgelink/service/data_processor.hpp"

namespace edgelink::service {

namespace {

MetricRange get_valid_range(model::MetricType type) noexcept {
    switch (type) {
        case model::MetricType::Temperature: return {-50.0, 150.0};
        case model::MetricType::Pressure:    return {0.0, 100.0};
        case model::MetricType::Voltage:     return {0.0, 500.0};
        case model::MetricType::Current:     return {0.0, 100.0};
        case model::MetricType::Vibration:   return {0.0, 50.0};
        case model::MetricType::Humidity:    return {0.0, 100.0};
    }
    return {-1e9, 1e9};
}

}

DataProcessor::DataProcessor(std::shared_ptr<TelemetryAggregator> aggregator)
    : aggregator_(std::move(aggregator)) {}

bool DataProcessor::validate(const model::TelemetryRecord& record) const noexcept {
    for (const auto& metric : record.metrics) {
        auto range = get_valid_range(metric.type);
        if (metric.value < range.min_val || metric.value > range.max_val) {
            return false;
        }
    }
    return true;
}

model::TelemetryRecord DataProcessor::normalize(const model::TelemetryRecord& record) const {
    model::TelemetryRecord normalized = record;
    for (auto& metric : normalized.metrics) {
        switch (metric.type) {
            case model::MetricType::Temperature:
                metric.value += 273.15;
                break;
            case model::MetricType::Pressure:
                metric.value *= 100000.0;
                break;
            default:
                break;
        }
    }
    return normalized;
}

void DataProcessor::process(const model::TelemetryRecord& record) {
    if (!validate(record)) {
        MetricsRegistry::instance().record_packet_out_of_range();
    }

    auto normalized = normalize(record);

    if (aggregator_) {
        aggregator_->update(normalized);
    }

    MetricsRegistry::instance().record_packet_processed();

    NormalizedOutputCallback cb;
    {
        std::lock_guard lock(callback_mutex_);
        cb = output_callback_;
    }
    if (cb) {
        cb(normalized);
    }
}

void DataProcessor::set_output_callback(NormalizedOutputCallback callback) {
    std::lock_guard lock(callback_mutex_);
    output_callback_ = std::move(callback);
}

std::shared_ptr<TelemetryAggregator> DataProcessor::aggregator() const noexcept {
    return aggregator_;
}

}
