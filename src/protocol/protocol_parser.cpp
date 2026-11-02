#include "edgelink/protocol/protocol_parser.hpp"
#include "edgelink/protocol/crc32.hpp"
#include <cstring>
#include <bit>
#include <algorithm>

namespace edgelink::protocol {

ParseResult ProtocolParser::parse_packet(std::span<const uint8_t> buffer) noexcept {
    if (buffer.size() < sizeof(PacketHeader) + sizeof(uint32_t)) {
        return {ParseStatus::NeedMoreData, 0, std::nullopt};
    }

    PacketHeader raw_hdr{};
    std::memcpy(&raw_hdr, buffer.data(), sizeof(PacketHeader));
    PacketHeader hdr = to_host_order(raw_hdr);

    if (hdr.magic != PROTOCOL_MAGIC) {
        return {ParseStatus::InvalidMagic, 1, std::nullopt};
    }

    if (hdr.version != PROTOCOL_VERSION) {
        return {ParseStatus::UnsupportedVersion, sizeof(PacketHeader), std::nullopt};
    }

    if (hdr.payload_length > MAX_PACKET_PAYLOAD_SIZE) {
        return {ParseStatus::PayloadTooLarge, sizeof(PacketHeader), std::nullopt};
    }

    const size_t total_size = sizeof(PacketHeader) + hdr.payload_length + sizeof(uint32_t);
    if (buffer.size() < total_size) {
        return {ParseStatus::NeedMoreData, 0, std::nullopt};
    }

    const size_t crc_offset = sizeof(PacketHeader) + hdr.payload_length;
    uint32_t expected_crc = 0;
    std::memcpy(&expected_crc, buffer.data() + crc_offset, sizeof(uint32_t));
    expected_crc = be32toh(expected_crc);

    const uint32_t actual_crc = Crc32::calculate(buffer.data(), sizeof(PacketHeader) + hdr.payload_length);
    if (actual_crc != expected_crc) {
        return {ParseStatus::ChecksumMismatch, sizeof(PacketHeader), std::nullopt};
    }

    model::TelemetryRecord record;
    record.device_id = hdr.device_id;
    record.timestamp_ms = hdr.timestamp_ms;

    const uint8_t* payload_ptr = buffer.data() + sizeof(PacketHeader);
    size_t remaining_payload = hdr.payload_length;

    while (remaining_payload > 0) {
        if (remaining_payload < 4) {
            return {ParseStatus::MalformedTlv, total_size, std::nullopt};
        }

        uint16_t raw_tag = 0;
        uint16_t raw_len = 0;
        std::memcpy(&raw_tag, payload_ptr, sizeof(uint16_t));
        std::memcpy(&raw_len, payload_ptr + 2, sizeof(uint16_t));

        uint16_t tag = be16toh(raw_tag);
        uint16_t len = be16toh(raw_len);

        payload_ptr += 4;
        remaining_payload -= 4;

        if (remaining_payload < len || len != sizeof(double)) {
            return {ParseStatus::MalformedTlv, total_size, std::nullopt};
        }

        uint64_t raw_val = 0;
        std::memcpy(&raw_val, payload_ptr, sizeof(uint64_t));
        raw_val = be64toh(raw_val);
        double val = std::bit_cast<double>(raw_val);

        record.metrics.push_back(model::TlvRecord{
            .type = static_cast<model::MetricType>(tag),
            .length = len,
            .value = val
        });

        payload_ptr += len;
        remaining_payload -= len;
    }

    return {ParseStatus::Success, total_size, record};
}

std::vector<model::TelemetryRecord> ProtocolParser::parse_stream(std::vector<uint8_t>& stream_buffer, size_t& corrupted_count) {
    std::vector<model::TelemetryRecord> records;

    while (stream_buffer.size() >= sizeof(PacketHeader) + sizeof(uint32_t)) {
        auto result = parse_packet(std::span<const uint8_t>(stream_buffer.data(), stream_buffer.size()));

        if (result.status == ParseStatus::Success) {
            if (result.record.has_value()) {
                records.push_back(std::move(*result.record));
            }
            stream_buffer.erase(stream_buffer.begin(), stream_buffer.begin() + result.bytes_consumed);
        } else if (result.status == ParseStatus::NeedMoreData) {
            break;
        } else {
            ++corrupted_count;
            size_t discard_bytes = std::max<size_t>(1, result.bytes_consumed);
            if (discard_bytes > stream_buffer.size()) {
                discard_bytes = stream_buffer.size();
            }
            stream_buffer.erase(stream_buffer.begin(), stream_buffer.begin() + discard_bytes);
        }
    }

    return records;
}

std::vector<uint8_t> ProtocolParser::serialize(const model::TelemetryRecord& record) {
    const uint32_t payload_len = static_cast<uint32_t>(record.metrics.size() * (sizeof(uint16_t) + sizeof(uint16_t) + sizeof(double)));
    const size_t total_packet_size = sizeof(PacketHeader) + payload_len + sizeof(uint32_t);

    std::vector<uint8_t> buffer(total_packet_size);

    PacketHeader hdr{
        .magic = PROTOCOL_MAGIC,
        .version = PROTOCOL_VERSION,
        .flags = 0,
        .device_id = record.device_id,
        .timestamp_ms = record.timestamp_ms,
        .payload_length = payload_len
    };

    PacketHeader net_hdr = to_network_order(hdr);
    std::memcpy(buffer.data(), &net_hdr, sizeof(PacketHeader));

    uint8_t* payload_ptr = buffer.data() + sizeof(PacketHeader);
    for (const auto& metric : record.metrics) {
        uint16_t tag = htobe16(static_cast<uint16_t>(metric.type));
        uint16_t len = htobe16(sizeof(double));
        uint64_t val = htobe64(std::bit_cast<uint64_t>(metric.value));

        std::memcpy(payload_ptr, &tag, sizeof(uint16_t));
        payload_ptr += sizeof(uint16_t);

        std::memcpy(payload_ptr, &len, sizeof(uint16_t));
        payload_ptr += sizeof(uint16_t);

        std::memcpy(payload_ptr, &val, sizeof(uint64_t));
        payload_ptr += sizeof(uint64_t);
    }

    uint32_t crc = Crc32::calculate(buffer.data(), sizeof(PacketHeader) + payload_len);
    uint32_t net_crc = htobe32(crc);
    std::memcpy(buffer.data() + sizeof(PacketHeader) + payload_len, &net_crc, sizeof(uint32_t));

    return buffer;
}

}
