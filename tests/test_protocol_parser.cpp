#include <gtest/gtest.h>
#include "edgelink/protocol/protocol_parser.hpp"

TEST(ProtocolParserTest, SerializeAndParseRoundTrip) {
    edgelink::model::TelemetryRecord original;
    original.device_id = 42;
    original.timestamp_ms = 1700000000123ULL;
    original.metrics.push_back({
        .type = edgelink::model::MetricType::Temperature,
        .length = sizeof(double),
        .value = 23.75
    });
    original.metrics.push_back({
        .type = edgelink::model::MetricType::Pressure,
        .length = sizeof(double),
        .value = 101.3
    });

    auto serialized = edgelink::protocol::ProtocolParser::serialize(original);
    ASSERT_FALSE(serialized.empty());

    auto result = edgelink::protocol::ProtocolParser::parse_packet(serialized);
    EXPECT_EQ(result.status, edgelink::protocol::ParseStatus::Success);
    ASSERT_TRUE(result.record.has_value());

    EXPECT_EQ(result.record->device_id, 42);
    EXPECT_EQ(result.record->timestamp_ms, 1700000000123ULL);
    ASSERT_EQ(result.record->metrics.size(), 2);

    auto temp = result.record->get_metric(edgelink::model::MetricType::Temperature);
    ASSERT_TRUE(temp.has_value());
    EXPECT_DOUBLE_EQ(*temp, 23.75);

    auto press = result.record->get_metric(edgelink::model::MetricType::Pressure);
    ASSERT_TRUE(press.has_value());
    EXPECT_DOUBLE_EQ(*press, 101.3);
}

TEST(ProtocolParserTest, IncompletePacketReturnsNeedMoreData) {
    edgelink::model::TelemetryRecord record;
    record.device_id = 1;
    record.timestamp_ms = 1000;
    record.metrics.push_back({
        .type = edgelink::model::MetricType::Voltage,
        .length = sizeof(double),
        .value = 220.0
    });

    auto serialized = edgelink::protocol::ProtocolParser::serialize(record);
    ASSERT_GT(serialized.size(), 10);

    std::span<const uint8_t> partial(serialized.data(), serialized.size() / 2);
    auto result = edgelink::protocol::ProtocolParser::parse_packet(partial);
    EXPECT_EQ(result.status, edgelink::protocol::ParseStatus::NeedMoreData);
}

TEST(ProtocolParserTest, CorruptedChecksumRejected) {
    edgelink::model::TelemetryRecord record;
    record.device_id = 1;
    record.timestamp_ms = 1000;
    record.metrics.push_back({
        .type = edgelink::model::MetricType::Voltage,
        .length = sizeof(double),
        .value = 220.0
    });

    auto serialized = edgelink::protocol::ProtocolParser::serialize(record);
    serialized.back() ^= 0xFF;

    auto result = edgelink::protocol::ProtocolParser::parse_packet(serialized);
    EXPECT_EQ(result.status, edgelink::protocol::ParseStatus::ChecksumMismatch);
}

TEST(ProtocolParserTest, StreamWithFragmentationAndConcatenation) {
    edgelink::model::TelemetryRecord r1{.device_id = 10, .timestamp_ms = 1000, .metrics = {{edgelink::model::MetricType::Current, 8, 5.5}}};
    edgelink::model::TelemetryRecord r2{.device_id = 20, .timestamp_ms = 2000, .metrics = {{edgelink::model::MetricType::Voltage, 8, 230.0}}};

    auto b1 = edgelink::protocol::ProtocolParser::serialize(r1);
    auto b2 = edgelink::protocol::ProtocolParser::serialize(r2);

    std::vector<uint8_t> stream;
    size_t corrupted = 0;

    stream.insert(stream.end(), b1.begin(), b1.begin() + 15);
    auto parsed = edgelink::protocol::ProtocolParser::parse_stream(stream, corrupted);
    EXPECT_TRUE(parsed.empty());
    EXPECT_EQ(corrupted, 0);
    EXPECT_EQ(stream.size(), 15);

    stream.insert(stream.end(), b1.begin() + 15, b1.end());
    stream.insert(stream.end(), b2.begin(), b2.end());

    parsed = edgelink::protocol::ProtocolParser::parse_stream(stream, corrupted);
    ASSERT_EQ(parsed.size(), 2);
    EXPECT_EQ(parsed[0].device_id, 10);
    EXPECT_EQ(parsed[1].device_id, 20);
    EXPECT_TRUE(stream.empty());
    EXPECT_EQ(corrupted, 0);
}

TEST(ProtocolParserTest, StreamResynchronizationOnCorruptedBytes) {
    edgelink::model::TelemetryRecord r{.device_id = 99, .timestamp_ms = 5000, .metrics = {{edgelink::model::MetricType::Temperature, 8, 36.6}}};
    auto valid_bytes = edgelink::protocol::ProtocolParser::serialize(r);

    std::vector<uint8_t> stream = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02};
    stream.insert(stream.end(), valid_bytes.begin(), valid_bytes.end());

    size_t corrupted = 0;
    auto parsed = edgelink::protocol::ProtocolParser::parse_stream(stream, corrupted);

    ASSERT_EQ(parsed.size(), 1);
    EXPECT_EQ(parsed[0].device_id, 99);
    EXPECT_GT(corrupted, 0);
    EXPECT_TRUE(stream.empty());
}
