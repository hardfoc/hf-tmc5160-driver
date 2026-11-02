# Changelog

All notable changes to this driver are documented here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and the project uses
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Host unit tests (`-DHF_TMC51X0_BUILD_TESTS=ON`) with a register-file model of the chip behind the SPI
  interface, and a CI workflow that runs them with GCC and Clang.

### Changed

- Documentation moved to the HardFOC MkDocs platform: new page set (quickstart, hardware, architecture, motion
  and units, current and chopper, StallGuard and homing, multi-chip, protocols, TMC5130, API guide, porting,
  testing), code snippets compiled by the host build from `tests/docs/`, Doxygen under `/api/`, versioned with
  mike. The Jekyll site files and the doxygen-awesome submodule are gone.

### Fixed

- `TMC51x0DaisyChain` and `TMC51x0MultiNode` compile: their constructors passed the clock frequency as the
  daisy-chain position and could not store the drivers; `IsDeviceActive()` and `MultiNode::InitializeAll()`
  returned the wrong types. `ProgramSequentially()` / `ProgramDevice()` now drive NAO low after writing
  `NODECONF` (TMC5160A datasheet section 5.4, figure 5.1).
- TMC5130: `Initialize()` reads the chip version before its first chip-specific write and no longer writes
  `SHORT_CONF`, `DRV_CONF` or `GLOBAL_SCALER`; the automatic current calculation uses the TMC5130 formula
  (RSENSE + 20 mOhm, VFS 0.32 V) instead of assuming a `GLOBAL_SCALER`, which set too much current.
  `status.VerifySetup()` accepts version 0x11.
- UART datagrams carry the full 8-bit node address (it was masked to 7 bits).
- `UartCommInterface::GetNaoPin()` calls the port's `GetNaoPin()` (it called a `GetNaoPin(bool&)` overload).
- `Unit::Mm` without a lead screw or belt returns `INVALID_VALUE` from the motion, ramp, `X_COMPARE` and homing
  calls instead of commanding position or speed 0. New `IsUnitConvertible()`.
- `motorControl.Enable()` / `Disable()` return the EN (DRV_ENN) GPIO error instead of ignoring it (`UNSUPPORTED`
  is still accepted); `Disable()` drives EN before touching the bus.
- `GetDriverVersion()`, `GetDriverVersionMajor/Minor/Patch()` and `IsInitialized()` are public.
- Install rules: `tmc51x0.ipp` is installed next to `tmc51x0.hpp` (included by name, no longer through
  `../src/`), the exported target is `hf::tmc51x0` like the in-tree alias, and `include/features` and
  `include/registers` are on the install interface.
- `MotorControl::IsStealthChopCalibrated()` and `Encoder::SetNChannelActiveLevel()`, `SetNChannelSensitivity()`,
  `SetClearMode()` and `SetPrescalerMode()` did not compile when used.

### Breaking changes

- `Enable()` and `Disable()` now fail when the port's `GpioSet(EN, ...)` fails. A port that has no EN pin must
  return `UNSUPPORTED` (as the porting contract says), not another error. The ESP32 example port now does.
- Calls that take `Unit::Mm` return `INVALID_VALUE` when `DriverConfig::mechanical` has no lead screw or belt,
  where they used to succeed and move to 0.
- On a TMC5130 the calculated `IRUN` / `IHOLD` change (lower, matching the requested current). Applications that
  compensated by calling `SetCurrent()` after `Initialize()` can drop that.
- The installed CMake target is `hf::tmc51x0` (was `hf::hf_tmc51x0`).
- The `f_clk` constructor argument of `TMC51x0DaisyChain` / `TMC51x0MultiNode` is ignored; the clock comes from
  `DriverConfig::external_clk_config`.

