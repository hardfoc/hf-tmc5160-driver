# Installs the driver from BUILD_DIR into a scratch prefix and builds tests/install against it.
# Usage: cmake -DBUILD_DIR=<build> -DSOURCE_DIR=<repo> -DCXX=<compiler> -P tests/install_check.cmake
set(prefix "${BUILD_DIR}/install-check/prefix")
set(consumer_build "${BUILD_DIR}/install-check/consumer")
file(REMOVE_RECURSE "${BUILD_DIR}/install-check")
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${BUILD_DIR}" --prefix "${prefix}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "install failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/install" -B "${consumer_build}"
                        "-DCMAKE_PREFIX_PATH=${prefix}" "-DCMAKE_CXX_COMPILER=${CXX}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "consumer configure failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${consumer_build}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "consumer build against the installed package failed")
endif()
