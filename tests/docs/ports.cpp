/**
 * @file ports.cpp
 * @brief Instantiates the documented SPI and UART ports with the driver, so both compile in full.
 */
#include "my_spi.hpp"
#include "my_uart.hpp"

template class tmc51x0::TMC51x0<MySpi>;
template class tmc51x0::TMC51x0<MyUart>;
