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

- `MotorControl::IsStealthChopCalibrated()` and `Encoder::SetNChannelActiveLevel()`, `SetNChannelSensitivity()`,
  `SetClearMode()` and `SetPrescalerMode()` did not compile when used.

