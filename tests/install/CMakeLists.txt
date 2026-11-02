# Consumer of an installed hf_tmc51x0 package (find_package), built by tests/install_check.cmake.
cmake_minimum_required(VERSION 3.16)
project(hf_tmc51x0_install_consumer LANGUAGES CXX)
find_package(hf_tmc51x0 REQUIRED CONFIG)
add_executable(consumer consumer.cpp)
target_link_libraries(consumer PRIVATE hf::tmc51x0)
