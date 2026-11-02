#include <gtest/gtest.h>
#include "edgelink/protocol/crc32.hpp"
#include <string>

TEST(Crc32Test, KnownVectors) {
    const std::string text = "123456789";
    uint32_t crc = edgelink::protocol::Crc32::calculate(text.data(), text.size());
    EXPECT_EQ(crc, 0xCBF43926u);
}

TEST(Crc32Test, EmptyData) {
    uint32_t crc = edgelink::protocol::Crc32::calculate("", 0);
    EXPECT_EQ(crc, 0x00000000u);
}

TEST(Crc32Test, DifferentPayloadsProduceDifferentCrc) {
    const std::string a = "PayloadAlpha";
    const std::string b = "PayloadBeta";
    EXPECT_NE(
        edgelink::protocol::Crc32::calculate(a.data(), a.size()),
        edgelink::protocol::Crc32::calculate(b.data(), b.size())
    );
}
