/**
 * @file my_spi.hpp
 * @brief The SPI port shown on the documentation's Porting page, compiled by the host build.
 *
 * Every platform call goes through the `platform_*` functions declared below; a real port replaces them with
 * its SDK (ESP-IDF, STM32 HAL, Zephyr, Linux spidev). The class is what the driver needs, nothing more.
 */
#pragma once

#include "tmc51x0.hpp"

#include <cstdarg>
#include <cstddef>
#include <cstdint>

// Platform hooks: your SDK's calls. Declared only, so the docs build checks the port compiles.
bool platform_spi_transfer(const uint8_t* tx, uint8_t* rx, std::size_t len) noexcept;  // CSN low .. CSN high
bool platform_gpio_write(int gpio, bool level) noexcept;
bool platform_gpio_read(int gpio, bool& level) noexcept;
void platform_delay_us(uint32_t us) noexcept;

// --8<-- [start:port]
class MySpi : public tmc51x0::SpiCommInterface<MySpi> {
public:
  tmc51x0::CommMode GetMode() const noexcept { return tmc51x0::CommMode::SPI; }

  // One full-duplex transfer of `length` bytes (5 per chip in the chain), SPI mode 3, CSN low throughout.
  tmc51x0::Result<void> SpiTransfer(const uint8_t* tx, uint8_t* rx, size_t length) noexcept {
    if (!platform_spi_transfer(tx, rx, length)) {
      return tmc51x0::Result<void>(tmc51x0::ErrorCode::COMM_ERROR);
    }
    return tmc51x0::Result<void>();
  }

  // ACTIVE / INACTIVE to a GPIO level. DRV_ENN is active low: ACTIVE (enabled) drives it low.
  tmc51x0::Result<void> GpioSet(tmc51x0::TMC51x0CtrlPin pin, tmc51x0::GpioSignal signal) noexcept {
    const int gpio = GpioOf(pin);
    const bool level = (signal == tmc51x0::GpioSignal::ACTIVE) == levels_.GetActiveLevel(pin);
    if (gpio < 0 || !platform_gpio_write(gpio, level)) {
      return tmc51x0::Result<void>(tmc51x0::ErrorCode::INVALID_VALUE);
    }
    return tmc51x0::Result<void>();
  }

  tmc51x0::Result<tmc51x0::GpioSignal> GpioRead(tmc51x0::TMC51x0CtrlPin pin) noexcept {
    bool level = false;
    const int gpio = GpioOf(pin);
    if (gpio < 0 || !platform_gpio_read(gpio, level)) {
      return tmc51x0::Result<tmc51x0::GpioSignal>(tmc51x0::ErrorCode::INVALID_VALUE);
    }
    return tmc51x0::Result<tmc51x0::GpioSignal>(level == levels_.GetActiveLevel(pin)
                                                    ? tmc51x0::GpioSignal::ACTIVE
                                                    : tmc51x0::GpioSignal::INACTIVE);
  }

  void DelayMs(uint32_t ms) noexcept { platform_delay_us(ms * 1000U); }
  void DelayUs(uint32_t us) noexcept { platform_delay_us(us); }

  // Route driver logs to your logger (level 0 = error ... 4 = verbose), or leave it empty.
  void DebugLog(int /*level*/, const char* /*tag*/, const char* /*format*/, va_list /*args*/) noexcept {}

private:
  static int GpioOf(tmc51x0::TMC51x0CtrlPin pin) noexcept {
    switch (pin) {
    case tmc51x0::TMC51x0CtrlPin::EN: return 6;      // DRV_ENN
    case tmc51x0::TMC51x0CtrlPin::DIAG0: return 7;   // optional: interrupt / stall output
    case tmc51x0::TMC51x0CtrlPin::DIAG1: return 8;   // optional: position compare output
    default: return -1;                              // not wired on this board
    }
  }

  tmc51x0::PinActiveLevels levels_{};  // defaults: DRV_ENN active low, the other pins active high
};
// --8<-- [end:port]
