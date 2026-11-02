/**
 * @file configuration.cpp
 * @brief The Configuration page's snippets, compiled by the host build (never run).
 */
#include "my_spi.hpp"
#include "tmc51x0_config_builder.hpp"

using tmc51x0::Unit;

tmc51x0::DriverConfig board_config() {
  // --8<-- [start:struct]
  tmc51x0::DriverConfig cfg{};

  // Motor and board: the driver derives GLOBAL_SCALER, IRUN and IHOLD from these.
  cfg.motor_spec.steps_per_rev = 200;
  cfg.motor_spec.rated_current_ma = 2800;
  cfg.motor_spec.winding_resistance_mohm = 1100;
  cfg.motor_spec.winding_inductance_mh = 2.6f;
  cfg.motor_spec.sense_resistor_mohm = 33;
  cfg.motor_spec.supply_voltage_mv = 36000;

  // Power stage (TMC5160 only: DRV_CONF, SHORT_CONF).
  cfg.power_stage.mosfet_miller_charge_nc = 8.0f;   // from the MOSFET datasheet, sets DRVSTRENGTH
  cfg.power_stage.bbm_time_ns = 100;                // break-before-make
  cfg.power_stage.over_temp_protection = tmc51x0::OverTempProtection::Temp150C;

  // Chopper: SpreadCycle with the datasheet's starting values, 256 microsteps.
  cfg.chopper.mode = tmc51x0::ChopperMode::SPREAD_CYCLE;
  cfg.chopper.toff = 5;
  cfg.chopper.tbl = 2;
  cfg.chopper.hstrt = 4;
  cfg.chopper.hend = 0;
  cfg.chopper.mres = tmc51x0::MicrostepResolution::MRES_256;

  // StealthChop at low speed (GCONF.en_pwm_mode), SpreadCycle above TPWMTHRS.
  cfg.global_config.en_stealthchop_mode = true;
  cfg.stealthchop.pwm_freq = 1;

  // Mechanics, for Unit::Mm: an 8 mm lead screw.
  cfg.mechanical.system_type = tmc51x0::MechanicalSystemType::LeadScrew;
  cfg.mechanical.lead_screw_pitch_mm = 8.0f;

  // Ramp defaults applied by Initialize().
  cfg.ramp_config.vmax = tmc51x0::VelocityValue::FromRPM(300.0f);
  cfg.ramp_config.amax = tmc51x0::AccelerationValue::FromRevPerSec(5.0f);
  return cfg;
  // --8<-- [end:struct]
}

tmc51x0::DriverConfig built_config() {
  // --8<-- [start:builder]
  return tmc51x0::ConfigBuilder()
      .WithMotorMa(200, 2800)
      .WithRunCurrentMa(2000)
      .WithSenseResistorMohm(33)
      .WithSupplyVoltageMv(36000)
      .WithChopperMode(tmc51x0::ChopperMode::SPREAD_CYCLE)
      .WithMicrostepResolution(tmc51x0::MicrostepResolution::MRES_256)
      .WithStealthChop(true)
      .WithStealthChopThreshold({120.0f, Unit::RPM})   // SpreadCycle above 120 RPM
      .WithMaxSpeed({300.0f, Unit::RPM})
      .WithAcceleration({5.0f, Unit::RevPerSec})
      .WithLeadScrew(8.0f)
      .Build();
  // --8<-- [end:builder]
}

void runtime_changes(tmc51x0::TMC51x0<MySpi>& driver) {
  // --8<-- [start:runtime]
  // Switch between StealthChop (quiet, low speed), SpreadCycle and full step by speed.
  driver.thresholds.SetModeChangeSpeeds(1.0f, 0.5f, 5.0f, Unit::RevPerSec);   // TPWMTHRS, TCOOLTHRS, THIGH

  // CoolStep: lower the current while the load is light (needs SpreadCycle and TCOOLTHRS).
  tmc51x0::CoolStepConfig cool{};
  cool.lower_threshold_sg = 100;
  cool.upper_threshold_sg = 300;
  cool.min_velocity = 0.5f;
  cool.velocity_unit = Unit::RevPerSec;
  driver.motorControl.ConfigureCoolStep(cool);

  // Change the run and hold currents (IRUN, IHOLD: 0..31 of the GLOBAL_SCALER full scale).
  driver.motorControl.SetCurrent(20, 8);
  // --8<-- [end:runtime]
}
