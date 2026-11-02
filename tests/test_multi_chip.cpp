/**
 * @file test_multi_chip.cpp
 * @brief The multi-chip helpers (TMC51x0DaisyChain over SPI, TMC51x0MultiNode over UART) and UART addressing.
 *
 * TMC5160A datasheet section 4.1 (SPI daisy chain), section 5.4 / figure 5.1 (UART node addressing with NAI/NAO)
 * and the NODECONF / OUTPUT register descriptions (0x03, 0x04).
 */
#include "test_harness.hpp"

#include "mock_tmc51x0.hpp"
#include "tmc51x0_daisy_chain.hpp"
#include "tmc51x0_multi_node.hpp"

#include <cstdarg>
#include <utility>
#include <vector>

namespace {

/// Records every UART datagram; reads are not answered (writes have no reply on this bus).
class MockUart : public tmc51x0::UartCommInterface<MockUart> {
public:
  tmc51x0::CommMode GetMode() const noexcept { return tmc51x0::CommMode::UART; }
  tmc51x0::Result<void> UartSend(const uint8_t* data, size_t length) noexcept {
    frames.emplace_back(data, data + length);
    return tmc51x0::Result<void>();
  }
  tmc51x0::Result<void> UartReceive(uint8_t* /*data*/, size_t /*length*/) noexcept {
    return tmc51x0::Result<void>(tmc51x0::ErrorCode::TIMEOUT);
  }
  tmc51x0::Result<void> SetNaiPin(bool /*active*/) noexcept { return tmc51x0::Result<void>(); }
  tmc51x0::Result<bool> GetNaoPin() noexcept { return tmc51x0::Result<bool>(true); }
  tmc51x0::Result<void> GpioSet(tmc51x0::TMC51x0CtrlPin /*pin*/, tmc51x0::GpioSignal /*signal*/) noexcept {
    return tmc51x0::Result<void>();
  }
  tmc51x0::Result<tmc51x0::GpioSignal> GpioRead(tmc51x0::TMC51x0CtrlPin /*pin*/) noexcept {
    return tmc51x0::Result<tmc51x0::GpioSignal>(tmc51x0::GpioSignal::INACTIVE);
  }
  void DelayMs(uint32_t /*ms*/) noexcept {}
  void DelayUs(uint32_t /*us*/) noexcept {}
  void DebugLog(int /*level*/, const char* /*tag*/, const char* /*format*/, va_list /*args*/) noexcept {}

  std::vector<std::vector<uint8_t>> frames;
};

}  // namespace

// Every member of the helpers is compiled, so a constructor or call that does not match TMC51x0 fails the build.
template class tmc51x0::TMC51x0DaisyChain<MockSpi, 4>;
template class tmc51x0::TMC51x0MultiNode<MockUart, 4>;

namespace tmc51x0_test {

TEST_CASE("TMC51x0DaisyChain: chips get positions 0..n-1 and the chain length follows them") {
  MockSpi spi;
  tmc51x0::TMC51x0DaisyChain<MockSpi, 4> chain(spi, 3);
  CHECK_EQ(chain.GetNumActiveDevices(), 3u);
  CHECK_EQ(spi.GetDaisyChainLength(), 3u);
  for (uint8_t i = 0; i < 3; ++i) {
    REQUIRE(chain.IsDeviceActive(i));
    CHECK_EQ(chain[i].communication.GetDaisyChainPosition(), i);
  }
  CHECK(!chain.IsDeviceActive(3));
  CHECK(!chain.IsDeviceActive(200));  // out of range: false, not an error code converted to bool
  REQUIRE(chain.AddDevice(3).IsOk());
  CHECK_EQ(spi.GetDaisyChainLength(), 4u);
  CHECK_EQ(chain[3].communication.GetDaisyChainPosition(), 3u);
  REQUIRE(chain.RemoveDevice(3).IsOk());
  CHECK_EQ(spi.GetDaisyChainLength(), 3u);
}

TEST_CASE("TMC51x0MultiNode: ProgramSequentially writes NODECONF at address 0, then drives NAO low") {
  // Figure 5.1: program the chip answering at 0 to 254 and set its NAO low (OUTPUT bit 0 = 0); the next chip
  // then answers at 0 and gets 253.
  MockUart uart;
  tmc51x0::TMC51x0MultiNode<MockUart, 4> nodes(uart, 2);
  REQUIRE(nodes.ProgramSequentially(2).IsOk());
  CHECK_EQ(nodes[0].communication.GetUartNodeAddress(), 254u);
  CHECK_EQ(nodes[1].communication.GetUartNodeAddress(), 253u);
  // Write datagram: sync, node address, register | 0x80, 4 data bytes, CRC.
  std::vector<std::pair<uint8_t, uint8_t>> node_reg;
  for (const auto& f : uart.frames) {
    REQUIRE(f.size() == 8u);
    node_reg.emplace_back(f[1], static_cast<uint8_t>(f[2] & 0x7F));
  }
  REQUIRE(node_reg.size() == 4u);
  CHECK_EQ(node_reg[0].first, 0u);
  CHECK_EQ(node_reg[0].second, tmc51x0::Registers::NODECONF);
  CHECK_EQ(uart.frames[0][6], 254u);  // NODEADDR in the low data byte
  CHECK_EQ(node_reg[1].first, 254u);
  CHECK_EQ(node_reg[1].second, tmc51x0::Registers::OUTPUT);
  CHECK_EQ(uart.frames[1][6] & 0x01u, 0u);  // NAO low
  CHECK_EQ(node_reg[2].first, 0u);
  CHECK_EQ(uart.frames[2][6], 253u);
  CHECK_EQ(node_reg[3].first, 253u);
}

TEST_CASE("UART datagrams carry the full 8-bit node address") {
  // NODECONF.NODEADDR is eight bits (0..253, 254 without NEXTADDR); the address byte is sent unmasked.
  MockUart uart;
  REQUIRE(uart.WriteRegister(tmc51x0::Registers::GCONF, 0, 200).IsOk());
  REQUIRE(uart.frames.size() == 1u);
  CHECK_EQ(uart.frames[0][1], 200u);
  (void)uart.ReadRegister(tmc51x0::Registers::GCONF, 254);
  REQUIRE(uart.frames.size() == 2u);
  CHECK_EQ(uart.frames[1][1], 254u);
}

TEST_CASE("UartCommInterface::GetNaoPin dispatches to the port's GetNaoPin()") {
  MockUart uart;
  auto& base = static_cast<tmc51x0::UartCommInterface<MockUart>&>(uart);
  auto r = base.GetNaoPin();
  REQUIRE(r.IsOk());
  CHECK(r.Value());
}

}  // namespace tmc51x0_test
