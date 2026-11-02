/**
 * @file mock_tmc51x0.hpp
 * @brief A register-file model of one TMC5160 / TMC5130 behind the SPI interface, for the host unit tests.
 *
 * The model follows the datasheet's SPI access rules (section 4.1): each 40-bit datagram is an address byte
 * (bit 7 set for a write) and 32 data bits, MSB first; the reply to a read arrives with the next datagram, and
 * the first byte of every reply is SPI_STATUS. IOIN carries the chip version in bits 31..24.
 */
#pragma once

#include "tmc51x0.hpp"

#include <array>
#include <cstdarg>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class MockSpi : public tmc51x0::SpiCommInterface<MockSpi> {
public:
  explicit MockSpi(uint8_t version = tmc51x0::ChipVersion::TMC5160) noexcept {
    regs_.fill(0);
    regs_[tmc51x0::Registers::IOIN] = static_cast<uint32_t>(version) << 24;  // DRV_ENN low, SD_MODE low
  }

  tmc51x0::CommMode GetMode() const noexcept { return tmc51x0::CommMode::SPI; }

  tmc51x0::Result<void> SpiTransfer(const uint8_t* tx, uint8_t* rx, size_t length) noexcept {
    ++transfers;
    lengths.push_back(length);
    if (length < 5) {
      return tmc51x0::Result<void>(tmc51x0::ErrorCode::COMM_ERROR);
    }
    // Reply: SPI_STATUS, then the data latched by the previous datagram.
    rx[0] = status;
    for (int i = 0; i < 4; ++i) {
      rx[1 + i] = static_cast<uint8_t>(pending_ >> (24 - 8 * i));
    }
    for (size_t i = 5; i < length; ++i) {
      rx[i] = 0;
    }
    // The command is the first five bytes (single chip: no padding behind it).
    const uint8_t addr = tx[0] & 0x7F;
    const uint32_t value = (static_cast<uint32_t>(tx[1]) << 24) | (static_cast<uint32_t>(tx[2]) << 16) |
                           (static_cast<uint32_t>(tx[3]) << 8) | static_cast<uint32_t>(tx[4]);
    if ((tx[0] & 0x80) != 0) {
      writes.emplace_back(addr, value);
      if (addr != tmc51x0::Registers::IOIN) {  // 0x04 write is OUTPUT; IOIN stays readable
        regs_[addr] = value;
      }
      pending_ = value;
    } else {
      pending_ = regs_[addr];
    }
    return tmc51x0::Result<void>();
  }

  tmc51x0::Result<void> GpioSet(tmc51x0::TMC51x0CtrlPin pin, tmc51x0::GpioSignal signal) noexcept {
    gpio_sets.emplace_back(pin, signal);
    return tmc51x0::Result<void>();
  }
  tmc51x0::Result<tmc51x0::GpioSignal> GpioRead(tmc51x0::TMC51x0CtrlPin /*pin*/) noexcept {
    return tmc51x0::Result<tmc51x0::GpioSignal>(tmc51x0::GpioSignal::INACTIVE);
  }
  void DelayMs(uint32_t ms) noexcept { elapsed_us += ms * 1000ULL; }
  void DelayUs(uint32_t us) noexcept { elapsed_us += us; }
  void DebugLog(int level, const char* tag, const char* /*format*/, va_list /*args*/) noexcept {
    if (level == static_cast<int>(tmc51x0::LogLevel::Warn)) {
      warnings.emplace_back(tag != nullptr ? tag : "");
    }
  }

  uint32_t& reg(uint8_t addr) noexcept { return regs_[addr & 0x7F]; }

  /// Last value written to a register, or false when it was never written.
  bool lastWrite(uint8_t addr, uint32_t& value) const noexcept {
    for (auto it = writes.rbegin(); it != writes.rend(); ++it) {
      if (it->first == addr) {
        value = it->second;
        return true;
      }
    }
    return false;
  }

  uint8_t status{0};
  int transfers{0};
  unsigned long long elapsed_us{0};
  std::vector<size_t> lengths;
  std::vector<std::pair<uint8_t, uint32_t>> writes;
  std::vector<std::pair<tmc51x0::TMC51x0CtrlPin, tmc51x0::GpioSignal>> gpio_sets;
  std::vector<std::string> warnings;  ///< Tags of the Warn-level log lines

private:
  std::array<uint32_t, 128> regs_{};
  uint32_t pending_{0};
};
