/**
 * @file multi_chip.cpp
 * @brief The Multi-chip page's snippets, compiled by the host build (never run).
 */
#include "my_spi.hpp"
#include "my_uart.hpp"

#include <cstdio>

void spi_chain(MySpi& spi, const tmc51x0::DriverConfig& cfg) {
  // --8<-- [start:spi-chain]
  spi.SetDaisyChainLength(3);                       // three chips share CSN, SCK and MOSI

  tmc51x0::TMC51x0<MySpi> x_axis(spi, 0);           // position 0: SDI on the MCU's MOSI
  tmc51x0::TMC51x0<MySpi> y_axis(spi, 1);
  tmc51x0::TMC51x0<MySpi> z_axis(spi, 2);           // position 2: SDO on the MCU's MISO

  for (auto* axis : {&x_axis, &y_axis, &z_axis}) {
    if (!axis->Initialize(cfg, false)) {
      std::printf("chip %u did not initialise\n", axis->communication.GetDaisyChainPosition());
    }
  }
  // --8<-- [end:spi-chain]
}

void uart_nodes(MyUart& uart, const tmc51x0::DriverConfig& cfg) {
  // --8<-- [start:uart-nodes]
  // First chip: NAI tied low, so it answers at address 0 after power-up.
  tmc51x0::TMC51x0<MyUart> first(uart, 0, 0);
  first.communication.ConfigureUartNodeAddress(1, 2);  // NODEADDR = 1, SENDDELAY >= 2 on a shared bus

  // Second chip: its NAI is driven by the first chip's NAO; it now answers at 0.
  tmc51x0::TMC51x0<MyUart> second(uart, 0, 0);
  second.communication.ConfigureUartNodeAddress(2, 2);

  first.Initialize(cfg, false);
  second.Initialize(cfg, false);
  // --8<-- [end:uart-nodes]
}
