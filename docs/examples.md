# Examples

Complete ESP32 applications that exercise the driver on real hardware, one feature area each.

They run on an ESP32-C6 wired to a TMC5160 board (the TMC5160-EVAL or a breakout board); the pins are in
[`esp32_tmc51x0_test_config.hpp`](../examples/esp32/main/test_config/esp32_tmc51x0_test_config.hpp) and the port in
[`esp32_tmc51x0_bus.hpp`](../examples/esp32/main/test_config/esp32_tmc51x0_bus.hpp). CI builds every app marked
"yes" below on each pull request.

## The apps

<!-- generated: examples -->
| App | What it shows | Source | CI |
| --- | --- | --- | --- |
| `internal_ramp_comprehensive_test` :material-star:{ title="featured" } | Internal-ramp test suite over SPI: core, motor control, ramp control, diagnostics and protection (one motor, encoder and reference switches) | [`internal_ramp_test_suites/internal_ramp_comprehensive_test.cpp`](../examples/esp32/main/internal_ramp_test_suites/internal_ramp_comprehensive_test.cpp) | yes |
| `spi_daisy_chain_comprehensive_test` | SPI daisy-chain test suite: chain setup, positions and multi-motor moves (set TEST_CHAIN_LENGTH to your hardware) | [`internal_ramp_test_suites/spi_daisy_chain_comprehensive_test.cpp`](../examples/esp32/main/internal_ramp_test_suites/spi_daisy_chain_comprehensive_test.cpp) | no |
| `uart_multi_node_comprehensive_test` | UART single-wire test suite: register access, node addressing, send delay and motion (set TEST_NODE_COUNT, default 1) | [`internal_ramp_test_suites/uart_multi_node_comprehensive_test.cpp`](../examples/esp32/main/internal_ramp_test_suites/uart_multi_node_comprehensive_test.cpp) | no |
| `internal_ramp_sinusoidal` :material-star:{ title="featured" } | Sinusoidal back-and-forth motion with the internal ramp generator | [`internal_ramp_test_suites/internal_ramp_sinusoidal.cpp`](../examples/esp32/main/internal_ramp_test_suites/internal_ramp_sinusoidal.cpp) | yes |
| `fatigue_test_espnow_unit` :material-star:{ title="featured" } | Fatigue test unit controlled over ESP-NOW or UART, with StallGuard2 or encoder bounds detection | [`fatigue_test_espnow/main.cpp`](../examples/esp32/main/fatigue_test_espnow/main.cpp) | no |
| `espnow_protocol_test_unit` | ESP-NOW protocol test unit: answers protocol messages without driving a motor | [`fatigue_test_espnow/espnow_protocol_test_unit.cpp`](../examples/esp32/main/fatigue_test_espnow/espnow_protocol_test_unit.cpp) | no |
| `stallguard_tuning` :material-star:{ title="featured" } | Finds the StallGuard2 threshold (SGT) for sensorless homing | [`tuning/stallguard_tuning.cpp`](../examples/esp32/main/tuning/stallguard_tuning.cpp) | yes |
| `bounds_finding_test` :material-star:{ title="featured" } | Finds the mechanical limits, homes to the centre and reports the travel | [`tuning/bounds_finding_test.cpp`](../examples/esp32/main/tuning/bounds_finding_test.cpp) | no |
| `abn_encoder_reader` :material-star:{ title="featured" } | Reads and prints the position of an ABN incremental encoder | [`sensors/abn_encoder_reader.cpp`](../examples/esp32/main/sensors/abn_encoder_reader.cpp) | no |
| `i2c_scan` | I2C bus scanner for the OLED UI board wiring (SDA = GPIO5, SCL = GPIO23) | [`diagnostics/i2c_scan.cpp`](../examples/esp32/main/diagnostics/i2c_scan.cpp) | no |

Build and flash one (from `examples/esp32`, ESP-IDF environment loaded):

```bash
./scripts/build_app.sh internal_ramp_comprehensive_test Release
./scripts/flash_app.sh flash_monitor internal_ramp_comprehensive_test Release
```

Source of this table: [`examples/esp32/app_config.yml`](../examples/esp32/app_config.yml).
<!-- /generated -->

## Set up once

```bash
git submodule update --init examples/esp32/scripts   # the HardFOC ESP-IDF build scripts
cd examples/esp32
./scripts/build_app.sh list                           # every app, build type and ESP-IDF version
```

The scripts read [`app_config.yml`](../examples/esp32/app_config.yml) (the same file this page's table comes
from) and build each app as its own firmware image. The [example folder's README](../examples/esp32/README.md)
covers installing ESP-IDF with the project tools.

## What to read in an example

| You want to | Read |
| --- | --- |
| See a full bring-up: currents, chopper, ramp, diagnostics, protection | `internal_ramp_comprehensive_test.cpp` |
| Move in a smooth back-and-forth pattern | `internal_ramp_sinusoidal.cpp` |
| Tune the StallGuard2 threshold on your mechanics | `stallguard_tuning.cpp` ([StallGuard and homing](stallguard-and-homing.md#tune-the-threshold)) |
| Find the ends of travel and home to the centre | `bounds_finding_test.cpp` |
| Read an ABN encoder | `abn_encoder_reader.cpp` |
| Run several chips on one SPI bus or one UART | `spi_daisy_chain_comprehensive_test.cpp`, `uart_multi_node_comprehensive_test.cpp` ([Multi-chip](multi-chip.md)) |
| Run a long fatigue test controlled over ESP-NOW | `fatigue_test_espnow/main.cpp` and its [`docs/`](../examples/esp32/main/fatigue_test_espnow/README.md) |

Each app has a page in [`examples/esp32/docs/`](../examples/esp32/docs/index.md) with its wiring and expected
output. The board, motor and platform settings the apps share are described in the
[driver configuration guide](../examples/esp32/docs/driver_configuration_guide.md).
