#pragma once

#include "edgelink/common/types.hpp"
#include "edgelink/model/telemetry_record.hpp"
#include "edgelink/protocol/packet_header.hpp"
#include <vector>
#include <span>
#include <optional>

namespace edgelink::protocol {

enum class ParseStatus {
    Success,
    NeedMoreData,
    InvalidMagic,
    UnsupportedVersion,
    PayloadTooLarge,
    ChecksumMismatch,
    MalformedTlv
};

struct ParseResult {
    ParseStatus status{ParseStatus::NeedMoreData};
    size_t bytes_consumed{0};
    std::optional<model::TelemetryRecord> record{std::nullopt};
};

class ProtocolParser {
public:
    static ParseResult parse_packet(std::span<const uint8_t> buffer) noexcept;
    static std::vector<model::TelemetryRecord> parse_stream(std::vector<uint8_t>& stream_buffer, size_t& corrupted_count);
    static std::vector<uint8_t> serialize(const model::TelemetryRecord& record);
};

}
