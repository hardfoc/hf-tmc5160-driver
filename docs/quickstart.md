# Quickstart

Turn a stepper motor half a revolution from your MCU: wire the SPI bus and DRV_ENN, write a small port class,
call six driver functions.

!!! warning "Before you power the motor"
    Set the current-limit of your bench supply to about the motor's rated current and keep the shaft free to
    turn. Never connect or disconnect the motor while VS is on: the inductive kick can destroy the MOSFETs.

## 1. Wire it

| MCU | TMC5160 | Notes |
| --- | --- | --- |
| SPI SCK, MOSI, MISO, CS | SCK_CFG2, SDI_CFG1, SDO_CFG0, CSN_CFG3 | SPI mode 3; up to 4 MHz on the internal clock, start at 1 MHz |
| GPIO out | DRV_ENN | High switches the power stage off; the driver drives it low in `Enable()` |
| VCC_IO supply | SPI_MODE | Strap high: SPI interface |
| GND | SD_MODE | Strap low: internal ramp generator |
| GND | CLK | Internal 12 MHz clock (or feed an external clock) |
| GND | GND | One ground for MCU, logic and motor supply |

VCC_IO is the MCU's I/O voltage (3.3 V or 5 V). [Hardware](hardware.md) has the full pin list, the UART wiring
and the power ranges.

## 2. Add the driver

Add the repository as a submodule and link the `hf_tmc51x0` CMake target (or include `inc/` and build with
C++20). [Installation](installation.md) covers CMake and ESP-IDF.

```bash
git submodule add https://github.com/hardfoc/hf-tmc5160-driver.git external/hf-tmc5160-driver
```

## 3. Write the port

The port is the only platform code. It moves bytes over SPI, sets and reads control pins and waits. Replace the
`platform_*` calls with your SDK's.

```cpp title="my_spi.hpp"
--8<-- "tests/docs/my_spi.hpp:port"
```

`GpioSet()` receives a state (`ACTIVE` / `INACTIVE`), not a level. `PinActiveLevels` maps it to a level:
DRV_ENN is active low, so `ACTIVE` (power stage on) drives the pin low. [Porting](porting.md) explains each
method and shows the UART variant.

## 4. Move the motor

```cpp title="main.cpp"
--8<-- "tests/docs/quickstart.cpp:quickstart"
```

What each step does:

1. `Initialize()` clears the reset flags, computes `GLOBAL_SCALER`, `IRUN` and `IHOLD` from the motor and board
   values, writes the chopper and StealthChop defaults, reads the chip version and selects positioning mode.
   Nothing else works until it returns success.
2. `SetMaxSpeed()` and `SetAcceleration()` convert RPM and rev/s² to the chip's `VMAX` and `AMAX` / `DMAX`.
3. `Enable()` sets DRV_ENN active through your port: the bridges are now driven.
4. `SetTargetPosition()` converts 180° to 25 600 microsteps (200 full steps × 256 / 2) and writes `XTARGET`; the
   ramp generator starts at once.
5. `IsTargetReached()` reads `position_reached` from `RAMP_STAT`. In real code, sleep between polls.

This example is built by the host CI, so it compiles against the current headers.

## If it does not work

| Symptom | First thing to check |
| --- | --- |
| `Initialize()` returns `COMM_ERROR` | SPI mode 3, CS low for the whole 5-byte datagram, SPI_MODE strapped high |
| Reads return `0x00000000` or `0xFFFFFFFF` | MISO wiring, a common ground, VCC_IO and VS present |
| Motor does not move after `Enable()` | DRV_ENN wiring and level, `SD_MODE` low, VS in range ([Troubleshooting](troubleshooting.md)) |
| Motor vibrates or loses steps | `rated_current_ma` and `sense_resistor_mohm` against your hardware ([Current and chopper](current-and-chopper.md)) |

## Next

- [API guide](api-guide.md): velocity mode, readings, StallGuard, homing, encoder, faults.
- [Examples](examples.md): complete ESP32 applications.
- [Motion and units](motion-and-units.md): the ramp generator and the unit conversions.
