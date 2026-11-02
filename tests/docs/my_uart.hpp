/**
 * @file my_uart.hpp
 * @brief The UART port shown on the documentation's Porting page, compiled by the host build.
 */
#pragma once

#include "tmc51x0.hpp"

#include <cstdarg>
#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_uart_write(const uint8_t* data, std::size_t len) noexcept;
bool platform_uart_read(uint8_t* data, std::size_t len, uint32_t timeout_ms) noexcept;  // all bytes or false
bool platform_gpio_write(int gpio, bool level) noexcept;
bool platform_gpio_read(int gpio, bool& level) noexcept;
void platform_delay_us(uint32_t us) noexcept;

// --8<-- [start:port]
class MyUart : public tmc51x0::UartCommInterface<MyUart> {
public:
  tmc51x0::CommMode GetMode() const noexcept { return tmc51x0::CommMode::UART; }

  // Send one datagram (8 bytes for a write, 4 for a read request). On a single-wire bus the
  // transmitted bytes echo back: discard them here, or read them away in UartReceive().
  tmc51x0::Result<void> UartSend(const uint8_t* data, size_t length) noexcept {
    return platform_uart_write(data, length) ? tmc51x0::Result<void>()
                                             : tmc51x0::Result<void>(tmc51x0::ErrorCode::COMM_ERROR);
  }

  // Block until exactly `length` bytes (an 8-byte reply) arrived, or fail.
  tmc51x0::Result<void> UartReceive(uint8_t* data, size_t length) noexcept {
    return platform_uart_read(data, length, 10) ? tmc51x0::Result<void>()
                                                : tmc51x0::Result<void>(tmc51x0::ErrorCode::TIMEOUT);
  }

  // NAI (SDI_CFG1) selects which chip answers at address 0 while addresses are programmed.
  tmc51x0::Result<void> SetNaiPin(bool active) noexcept {
    return platform_gpio_write(kNaiGpio, active) ? tmc51x0::Result<void>()
                                                 : tmc51x0::Result<void>(tmc51x0::ErrorCode::COMM_ERROR);
  }

  tmc51x0::Result<bool> GetNaoPin() noexcept {
    return tmc51x0::Result<bool>(tmc51x0::ErrorCode::UNSUPPORTED);  // NAO not wired back on this board
  }

  tmc51x0::Result<void> GpioSet(tmc51x0::TMC51x0CtrlPin pin, tmc51x0::GpioSignal signal) noexcept {
    if (pin != tmc51x0::TMC51x0CtrlPin::EN) {
      return tmc51x0::Result<void>(tmc51x0::ErrorCode::UNSUPPORTED);
    }
    const bool enabled = signal == tmc51x0::GpioSignal::ACTIVE;
    return platform_gpio_write(kDrvEnnGpio, !enabled) ? tmc51x0::Result<void>()  // DRV_ENN: low = enabled
                                                      : tmc51x0::Result<void>(tmc51x0::ErrorCode::COMM_ERROR);
  }

  tmc51x0::Result<tmc51x0::GpioSignal> GpioRead(tmc51x0::TMC51x0CtrlPin /*pin*/) noexcept {
    return tmc51x0::Result<tmc51x0::GpioSignal>(tmc51x0::ErrorCode::UNSUPPORTED);
  }

  void DelayMs(uint32_t ms) noexcept { platform_delay_us(ms * 1000U); }
  void DelayUs(uint32_t us) noexcept { platform_delay_us(us); }
  void DebugLog(int /*level*/, const char* /*tag*/, const char* /*format*/, va_list /*args*/) noexcept {}

private:
  static constexpr int kNaiGpio = 9;
  static constexpr int kDrvEnnGpio = 6;
};
// --8<-- [end:port]
