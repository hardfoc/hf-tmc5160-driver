/**
 * @file quickstart.cpp
 * @brief The Quickstart page's example, compiled by the host build (never run).
 */
#include "my_spi.hpp"

// --8<-- [start:quickstart]
#include "tmc51x0.hpp"

int move_half_a_turn(MySpi& spi) {
  tmc51x0::TMC51x0<MySpi> driver(spi);

  tmc51x0::DriverConfig cfg{};                     // 1. motor and board
  cfg.motor_spec.rated_current_ma = 1500;          //    motor datasheet, RMS
  cfg.motor_spec.sense_resistor_mohm = 50;         //    R_SENSE on your board
  cfg.motor_spec.supply_voltage_mv = 24000;        //    VS
  if (!driver.Initialize(cfg)) {
    return -1;                                     // wiring, power or SPI mode
  }

  driver.rampControl.SetMaxSpeed(60.0f, tmc51x0::Unit::RPM);          // 2. ramp
  driver.rampControl.SetAcceleration(2.0f, tmc51x0::Unit::RevPerSec);  //    rev/s²
  driver.motorControl.Enable();                                        // 3. DRV_ENN low
  driver.rampControl.SetTargetPosition(180.0f, tmc51x0::Unit::Deg);    // 4. move

  for (;;) {                                       // 5. wait for position_reached
    auto reached = driver.rampControl.IsTargetReached();
    if (!reached) return -2;
    if (reached.Value()) return 0;
  }
}
// --8<-- [end:quickstart]
