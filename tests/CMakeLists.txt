# =============================================================================
# hf-tmc5160-driver — host unit tests
# =============================================================================
# Builds the driver headers against a model of the chip behind the SPI interface and runs the tests
# with CTest. No hardware and no ESP-IDF needed:
#
#   cmake -S . -B build-host -DHF_TMC51X0_BUILD_TESTS=ON
#   cmake --build build-host
#   ctest --test-dir build-host --output-on-failure
# =============================================================================

add_executable(hf_tmc51x0_host_tests
  test_main.cpp
  test_frames.cpp
  test_driver.cpp
  test_multi_chip.cpp
)

target_link_libraries(hf_tmc51x0_host_tests PRIVATE ${HF_TMC51X0_TARGET_NAME})
target_include_directories(hf_tmc51x0_host_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(hf_tmc51x0_host_tests PRIVATE cxx_std_20)
target_compile_options(hf_tmc51x0_host_tests PRIVATE -Wall -Wextra)

add_test(NAME hf_tmc51x0_host_tests COMMAND hf_tmc51x0_host_tests)

# The code shown on the documentation site (docs/ includes these files with pymdownx.snippets), built so a
# page cannot show code that does not compile against the current headers. Compiled, never linked or run.
add_library(hf_tmc51x0_doc_snippets OBJECT
  docs/quickstart.cpp
  docs/api_tasks.cpp
  docs/ports.cpp
  docs/configuration.cpp
  docs/multi_chip.cpp
)
target_link_libraries(hf_tmc51x0_doc_snippets PRIVATE ${HF_TMC51X0_TARGET_NAME})
target_include_directories(hf_tmc51x0_doc_snippets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
target_compile_features(hf_tmc51x0_doc_snippets PRIVATE cxx_std_20)
target_compile_options(hf_tmc51x0_doc_snippets PRIVATE -Wall -Wextra)

# The installed package must be usable: install this build tree and compile a find_package() consumer.
add_test(NAME hf_tmc51x0_install_consumer
  COMMAND ${CMAKE_COMMAND} -DBUILD_DIR=${CMAKE_BINARY_DIR} -DSOURCE_DIR=${PROJECT_SOURCE_DIR}
          -DCXX=${CMAKE_CXX_COMPILER} -P ${CMAKE_CURRENT_SOURCE_DIR}/install_check.cmake)
