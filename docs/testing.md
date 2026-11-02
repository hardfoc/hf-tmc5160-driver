# Testing and CI

How the driver is checked: host unit tests against a model of the chip, compiled documentation, ESP32 builds and
static analysis, on every pull request.

## Host unit tests

The tests build the real driver against a register-file model of one chip behind `SpiCommInterface`. The model
follows the datasheet's SPI rules: 40-bit datagrams, the reply to a read arriving with the next datagram,
`SPI_STATUS` in every reply, and the chip version in `IOIN`. Time is virtual, so tests run in milliseconds.

```bash
cmake -S . -B build-host -DHF_TMC51X0_BUILD_TESTS=ON
cmake --build build-host
ctest --test-dir build-host --output-on-failure
```

| File | Covers |
| --- | --- |
| [`tests/test_frames.cpp`](../tests/test_frames.cpp) | SPI datagram encoding against the datasheet examples, `SPI_STATUS` decoding, UART frames and CRC8 against the datasheet algorithm |
| [`tests/test_driver.cpp`](../tests/test_driver.cpp) | Pipelined reads, write confirmation, `Result<T>`, `Initialize()` with TMC5160 and TMC5130 detection (no TMC5160-only writes, TMC5130 current formula), `VerifySetup()`, motion commands and unit conversion (millimetres rejected without a mechanical system), `Enable()` / `Disable()` GPIO errors |
| [`tests/test_multi_chip.cpp`](../tests/test_multi_chip.cpp) | `TMC51x0DaisyChain` and `TMC51x0MultiNode` (explicitly instantiated), the NODECONF / NAO addressing sequence, 8-bit UART node addresses |
| [`tests/install_check.cmake`](../tests/install_check.cmake) | Installs the build tree and builds a `find_package(hf_tmc51x0)` consumer ([`tests/install/`](../tests/install/CMakeLists.txt)) |
| [`tests/mock_tmc51x0.hpp`](../tests/mock_tmc51x0.hpp) | The chip model |

## Documentation snippets

Every C++ block on this site is included from [`tests/docs/`](../tests/docs/my_spi.hpp), which the host build
compiles (target `hf_tmc51x0_doc_snippets`) with the driver explicitly instantiated over the documented SPI and
UART ports, so every member of `TMC51x0` is compiled. A change to the API that breaks a page breaks the build
first.

## CI workflows

| Workflow | What it checks |
| --- | --- |
| Host unit tests | GCC and Clang build and run the tests and the doc snippets |
| ESP32 examples build | Every app in `examples/esp32/app_config.yml` with CI enabled, for each listed ESP-IDF version and build type |
| C++ lint, C++ analysis | clang-format, clang-tidy and cppcheck on `inc/` and `src/` |
| Docs | Strict site build, figure and generated-table drift, Doxygen, link check, Lighthouse budget |
| Markdown lint, YAML lint, link check | Repository text files |

The workflows call the shared, versioned workflows of
[hf-general-ci-tools](https://github.com/hardfoc/hf-general-ci-tools) and
[hf-espidf-ci-tools](https://github.com/hardfoc/hf-espidf-ci-tools).

## On hardware

The [ESP32 examples](examples.md) are the hardware test suite: `internal_ramp_comprehensive_test` prints a pass /
fail summary over the serial monitor. Run it after any change to the transport or the register code.
