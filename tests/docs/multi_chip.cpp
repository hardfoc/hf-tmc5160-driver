/**
 * @file multi_chip.cpp
 * @brief The Multi-chip page's snippets, compiled by the host build (never run).
 */
#include "my_spi.hpp"
#include "my_uart.hpp"
#include "tmc51x0_daisy_chain.hpp"
#include "tmc51x0_multi_node.hpp"

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
  // After power-up the first chip (NAI tied low) answers at address 0; the others answer at 1.
  tmc51x0::TMC51x0<MyUart> first(uart);
  first.uartConfig.ConfigureUartNodeAddress(254, 2);   // NODECONF via address 0; SENDDELAY 2 for a shared bus
  first.io.SetSdoCfg0Polarity(false);                  // NAO low: the next chip now answers at 0

  tmc51x0::TMC51x0<MyUart> second(uart);
  second.uartConfig.ConfigureUartNodeAddress(253, 2);
  second.io.SetSdoCfg0Polarity(false);

  for (auto* node : {&first, &second}) {
    if (!node->Initialize(cfg, false)) {
      std::printf("node %u did not initialise\n", node->communication.GetUartNodeAddress());
    }
  }
  // --8<-- [end:uart-nodes]
}

void spi_chain_helper(MySpi& spi, const tmc51x0::DriverConfig& cfg) {
  // --8<-- [start:spi-chain-helper]
  tmc51x0::TMC51x0DaisyChain<MySpi, 4> chain(spi, 3);   // positions 0..2; sets the chain length to 3
  if (!chain.InitializeAll(cfg)) {
    std::printf("a chip in the chain did not initialise\n");
  }
  chain[2].rampControl.SetTargetPosition(90.0f, tmc51x0::Unit::Deg);
  // --8<-- [end:spi-chain-helper]
}

void uart_nodes_helper(MyUart& uart, const tmc51x0::DriverConfig& cfg) {
  // --8<-- [start:uart-nodes-helper]
  tmc51x0::TMC51x0MultiNode<MyUart, 4> nodes(uart, 3);
  if (nodes.ProgramSequentially(2) && nodes.InitializeAll(cfg)) {   // addresses 254, 253, 252
    nodes[1].rampControl.SetTargetPosition(90.0f, tmc51x0::Unit::Deg);
  }
  // --8<-- [end:uart-nodes-helper]
}
