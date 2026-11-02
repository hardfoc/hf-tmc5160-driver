# Installation

Add the driver to a CMake or ESP-IDF project. It is a header-only C++20 library with no dependencies.

## Requirements

- A C++20 compiler: GCC 10+, Clang 12+ or a current MSVC. The CMake target requires `cxx_std_20`.
- No RTOS and no exceptions: calls are `noexcept` and return `Result<T>`. The core allocates nothing on the heap;
  the string helpers that do (`GetDriverConfigString()`) disappear with `TMC51X0_DISABLE_DEBUG_LOGGING`.

## Get the source

A git submodule keeps the driver pinned to a version you chose:

```bash
git submodule add https://github.com/hardfoc/hf-tmc5160-driver.git external/hf-tmc5160-driver
git submodule update --init external/hf-tmc5160-driver
```

Only `inc/`, `src/`, `cmake/` and `CMakeLists.txt` are needed to build. The examples have their own submodules
(`examples/esp32/scripts` and two remote-controller projects); you do not need them to use the driver.

## CMake

```cmake
add_subdirectory(external/hf-tmc5160-driver)
target_link_libraries(my_app PRIVATE hf_tmc51x0)     # or the alias hf::tmc51x0
```

The `hf_tmc51x0` target carries the include directories (`inc/`, `inc/features/`, `inc/registers/`, `src/` and
the generated version header) and requires C++20, so your target does not need to set either.

| Option | Default | Effect |
| --- | --- | --- |
| `HF_TMC51X0_ENABLE_WARNINGS` | `OFF` | `-Wall -Wextra -Wpedantic` on targets that link the driver |
| `HF_TMC51X0_BUILD_TESTS` | `OFF` | Builds the host unit tests in `tests/` ([Testing and CI](testing.md)) |

## ESP-IDF

The ESP-IDF component wrapper is [`examples/esp32/components/hf_tmc51x0/`](../examples/esp32/components/hf_tmc51x0/CMakeLists.txt).
Copy that folder into your project's `components/` (or point `EXTRA_COMPONENT_DIRS` at it) and set the
`HF_TMC51X0_ROOT` cache variable to the driver checkout; the wrapper stops with a clear message when the path is
wrong. Your component then declares `REQUIRES hf_tmc51x0`. The wrapper depends on the IDF `driver` component.

## How the build is organised

| Layer | File | Role |
| --- | --- | --- |
| Settings | [`cmake/hf_tmc51x0_build_settings.cmake`](../cmake/hf_tmc51x0_build_settings.cmake) | Target name, include directories, version: the single source of truth |
| Library | [`CMakeLists.txt`](../CMakeLists.txt) | Interface library, install rules, tests option |
| ESP-IDF | [`examples/esp32/components/hf_tmc51x0/CMakeLists.txt`](../examples/esp32/components/hf_tmc51x0/CMakeLists.txt) | IDF component wrapper over the same settings |

The version is set once in the settings file and written into `tmc51x0_version.h` at configure time, which
defines `HF_TMC51X0_VERSION_STRING` (for example `"1.0.0"`) and the major, minor and patch numbers.

## Compile-time switches

| Define | Effect |
| --- | --- |
| `TMC51X0_DISABLE_DEBUG_LOGGING` | Removes every log call, the `Log*Summary()` helpers and `GetDriverConfigString()` |
| `TMC51X0_LOG_LEVEL=n` | Compiles in log calls up to level n (0 error, 1 warning, 2 info, 3 debug, 4 verbose; default 4) |
| `TMC51X0_NO_ERROR_STRINGS` | Removes `ErrorMessage()` and its string table |
| `TMC51X0_SPI_MAX_CHAIN_DEVICES=n` | Longest SPI daisy chain the interface accepts (default 8); sizes its scratch buffers |

## Next

[Quickstart](quickstart.md) wires the chip and moves a motor; [Porting](porting.md) writes the communication
interface for your MCU.
