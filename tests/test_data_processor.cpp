#include <gtest/gtest.h>
#include "edgelink/service/data_processor.hpp"

TEST(DataProcessorTest, ValidatePhysicallySensibleRanges) {
    edgelink::service::DataProcessor processor;

    edgelink::model::TelemetryRecord valid_record{
        .device_id = 1,
        .timestamp_ms = 1000,
        .metrics = {
            {edgelink::model::MetricType::Temperature, 8, 25.0},
            {edgelink::model::MetricType::Pressure, 8, 10.0}
        }
    };
    EXPECT_TRUE(processor.validate(valid_record));

    edgelink::model::TelemetryRecord invalid_record{
        .device_id = 2,
        .timestamp_ms = 1000,
        .metrics = {
            {edgelink::model::MetricType::Temperature, 8, 250.0}
        }
    };
    EXPECT_FALSE(processor.validate(invalid_record));
}

TEST(DataProcessorTest, NormalizationConversion) {
    edgelink::service::DataProcessor processor;

    edgelink::model::TelemetryRecord raw{
        .device_id = 5,
        .timestamp_ms = 2000,
        .metrics = {
            {edgelink::model::MetricType::Temperature, 8, 0.0},
            {edgelink::model::MetricType::Pressure, 8, 2.0}
        }
    };

    auto norm = processor.normalize(raw);
    auto temp = norm.get_metric(edgelink::model::MetricType::Temperature);
    ASSERT_TRUE(temp.has_value());
    EXPECT_NEAR(*temp, 273.15, 0.001);

    auto press = norm.get_metric(edgelink::model::MetricType::Pressure);
    ASSERT_TRUE(press.has_value());
    EXPECT_NEAR(*press, 200000.0, 0.001);
}

TEST(DataProcessorTest, AggregationStatistics) {
    auto aggregator = std::make_shared<edgelink::service::TelemetryAggregator>();
    edgelink::service::DataProcessor processor(aggregator);

    edgelink::model::TelemetryRecord r1{.device_id = 10, .timestamp_ms = 100, .metrics = {{edgelink::model::MetricType::Voltage, 8, 220.0}}};
    edgelink::model::TelemetryRecord r2{.device_id = 10, .timestamp_ms = 200, .metrics = {{edgelink::model::MetricType::Voltage, 8, 240.0}}};

    processor.process(r1);
    processor.process(r2);

    auto stats = aggregator->get_device_stats(10);
    ASSERT_TRUE(stats.has_value());
    EXPECT_EQ(stats->total_packets, 2);

    auto v_summary = stats->metrics.at(edgelink::model::MetricType::Voltage);
    EXPECT_DOUBLE_EQ(v_summary.min, 220.0);
    EXPECT_DOUBLE_EQ(v_summary.max, 240.0);
    EXPECT_DOUBLE_EQ(v_summary.average(), 230.0);
    EXPECT_DOUBLE_EQ(v_summary.last_value, 240.0);
}

TEST(DataProcessorTest, OutputCallbackInvoked) {
    edgelink::service::DataProcessor processor;
    std::atomic<bool> invoked{false};
    uint32_t received_dev = 0;

    processor.set_output_callback([&invoked, &received_dev](const edgelink::model::TelemetryRecord& rec) {
        invoked = true;
        received_dev = rec.device_id;
    });

    edgelink::model::TelemetryRecord r{.device_id = 77, .timestamp_ms = 1000, .metrics = {}};
    processor.process(r);

    EXPECT_TRUE(invoked.load());
    EXPECT_EQ(received_dev, 77);
}
