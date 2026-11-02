/**
 * @file test_frames.cpp
 * @brief SPI datagram and UART frame encoding against the datasheet (sections 4.1 and 5.1).
 */
#include "test_harness.hpp"

#include "tmc51x0_comm_interface.hpp"

#include <cstdint>

namespace {

// The CRC8 of the datasheet's UART section (polynomial x^8 + x^2 + x + 1, each byte LSB first),
// written out independently of the driver.
uint8_t datasheetCrc(const uint8_t* data, size_t len) {
  uint8_t crc = 0;
  for (size_t i = 0; i < len; ++i) {
    uint8_t byte = data[i];
    for (int j = 0; j < 8; ++j) {
      if (((crc >> 7) ^ (byte & 0x01)) != 0) {
        crc = static_cast<uint8_t>((crc << 1) ^ 0x07);
      } else {
        crc = static_cast<uint8_t>(crc << 1);
      }
      byte = static_cast<uint8_t>(byte >> 1);
    }
  }
  return crc;
}

} // namespace

TEST_CASE("SPI write datagram: address | 0x80, data MSB first") {
  // Datasheet example: write VMAX := 0x00ABCDEF -> 0xA700ABCDEF
  const auto cmd = tmc51x0::SpiCommand::Write(0x27, 0x00ABCDEF);
  uint8_t bytes[5]{};
  cmd.GetFrame(bytes);
  CHECK_EQ(bytes[0], 0xA7);
  CHECK_EQ(bytes[1], 0x00);
  CHECK_EQ(bytes[2], 0xAB);
  CHECK_EQ(bytes[3], 0xCD);
  CHECK_EQ(bytes[4], 0xEF);
  CHECK(cmd.IsWrite());
  CHECK_EQ(cmd.GetAddress(), 0x27);
  CHECK_EQ(cmd.GetValue(), 0x00ABCDEFu);
}

TEST_CASE("SPI read datagram: address with bit 7 clear, zero data") {
  // Datasheet example: read XACTUAL -> 0x2100000000
  const auto cmd = tmc51x0::SpiCommand::Read(0x21);
  uint8_t bytes[5]{0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  cmd.GetFrame(bytes);
  CHECK_EQ(bytes[0], 0x21);
  CHECK_EQ(bytes[1] | bytes[2] | bytes[3] | bytes[4], 0);
  CHECK(!cmd.IsWrite());
}

TEST_CASE("SPI_STATUS bits decode as in datasheet 4.1.2") {
  const auto s = tmc51x0::SpiStatus::FromByte(0x23);  // position_reached, driver_error, reset_flag
  CHECK(s.ResetFlag());
  CHECK(s.DriverError());
  CHECK(s.HasError());
  CHECK(s.PositionReached());
  CHECK(!s.StallGuard2());
  CHECK(!s.Standstill());
  CHECK(!s.VelocityReached());
  CHECK(!s.StopLeft());
  CHECK(!s.StopRight());
}

TEST_CASE("UART write frame: sync 0x05, node, address | 0x80, data, CRC") {
  const auto frame = tmc51x0::UartFrame::Write(3, 0x10, 0x00071F0A);
  uint8_t bytes[8]{};
  frame.GetFrame(bytes);
  CHECK_EQ(frame.GetSize(), 8u);
  CHECK_EQ(bytes[0], 0x05);
  CHECK_EQ(bytes[1], 3);
  CHECK_EQ(bytes[2], 0x90);
  CHECK_EQ(bytes[3], 0x00);
  CHECK_EQ(bytes[4], 0x07);
  CHECK_EQ(bytes[5], 0x1F);
  CHECK_EQ(bytes[6], 0x0A);
  CHECK_EQ(bytes[7], datasheetCrc(bytes, 7));
  CHECK(frame.VerifyCrc());
}

TEST_CASE("UART read request: four bytes, CRC over the first three") {
  const auto frame = tmc51x0::UartFrame::ReadRequest(0, 0x6F);
  uint8_t bytes[4]{};
  frame.GetFrame(bytes);
  CHECK_EQ(frame.GetSize(), 4u);
  CHECK_EQ(bytes[0], 0x05);
  CHECK_EQ(bytes[1], 0);
  CHECK_EQ(bytes[2], 0x6F);
  CHECK_EQ(bytes[3], datasheetCrc(bytes, 3));
}

TEST_CASE("UART read reply: a corrupted byte fails the CRC") {
  uint8_t reply[8] = {0x05, 0xFF, 0x6F, 0x12, 0x34, 0x56, 0x78, 0};
  reply[7] = datasheetCrc(reply, 7);
  auto ok = tmc51x0::UartFrame::ReadReply(reply);
  CHECK(ok.VerifyCrc());
  CHECK_EQ(ok.GetValue(), 0x12345678u);
  reply[4] ^= 0x01;
  auto bad = tmc51x0::UartFrame::ReadReply(reply);
  CHECK(!bad.VerifyCrc());
}
