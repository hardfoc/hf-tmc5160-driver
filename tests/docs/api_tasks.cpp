/**
 * @file api_tasks.cpp
 * @brief The API guide's task snippets, compiled by the host build (never run).
 */
#include "my_spi.hpp"

#include <cstdio>

using Driver = tmc51x0::TMC51x0<MySpi>;
using tmc51x0::Unit;

bool initialise(Driver& driver) {
  // --8<-- [start:init]
  tmc51x0::DriverConfig cfg{};
  cfg.motor_spec.steps_per_rev = 200;              // 1.8° motor
  cfg.motor_spec.rated_current_ma = 2000;          // RMS, from the motor datasheet
  cfg.motor_spec.run_current_ma = 1500;            // 0 = rated current
  cfg.motor_spec.hold_current_ma = 500;            // 0 = 30 % of run current
  cfg.motor_spec.sense_resistor_mohm = 50;         // 0.05 ohm
  cfg.motor_spec.supply_voltage_mv = 24000;
  cfg.chopper.mres = tmc51x0::MicrostepResolution::MRES_256;

  auto r = driver.Initialize(cfg, /*verbose=*/false);
  if (!r) {
    std::printf("init failed: %s\n", r.ErrorMessage());
    return false;
  }
  return driver.motorControl.Enable().IsOk();
  // --8<-- [end:init]
}

void position(Driver& driver) {
  // --8<-- [start:position]
  driver.rampControl.SetRampMode(tmc51x0::RampMode::POSITIONING);
  driver.rampControl.SetMaxSpeed(120.0f, Unit::RPM);
  driver.rampControl.SetAccelerations(4.0f, 4.0f, Unit::RevPerSec);   // accel, decel in rev/s²

  driver.rampControl.SetTargetPosition(90.0f, Unit::Deg);            // absolute
  driver.rampControl.MoveRelative(-45.0f, Unit::Deg);                // from the current target
  driver.rampControl.SetTargetPosition(200.0f, Unit::Steps);         // full steps: one turn of a 1.8° motor
  // --8<-- [end:position]
}

void velocity(Driver& driver) {
  // --8<-- [start:velocity]
  driver.rampControl.SetRampMode(tmc51x0::RampMode::VELOCITY_POS);   // or VELOCITY_NEG
  driver.rampControl.SetAcceleration(2.0f, Unit::RevPerSec);
  driver.rampControl.SetMaxSpeed(1.5f, Unit::RevPerSec);              // runs until told otherwise

  driver.rampControl.Stop();                                          // decelerate to zero with AMAX
  // --8<-- [end:velocity]
}

void read(Driver& driver) {
  // --8<-- [start:read]
  auto pos = driver.rampControl.GetCurrentPosition(Unit::Deg);       // XACTUAL
  auto rpm = driver.rampControl.GetCurrentSpeed(Unit::RPM);          // VACTUAL
  auto [err, standstill] = driver.rampControl.IsStandstill();       // structured binding
  if (pos && rpm && err == tmc51x0::ErrorCode::OK) {
    std::printf("%.1f deg, %.1f rpm, %s\n", pos.Value(), rpm.Value(), standstill ? "stopped" : "moving");
  }
  const uint16_t load = driver.stallGuard.GetStallGuardResult().ValueOr(0);   // SG_RESULT, 0..1023
  std::printf("load %u\n", load);
  // --8<-- [end:read]
}

void diagnose(Driver& driver) {
  // --8<-- [start:errors]
  bool drv_err = false;
  bool uv_cp = false;
  auto reset = driver.status.GetGlobalStatus(drv_err, uv_cp);        // GSTAT: true = reset occurred
  if (reset.IsOk() && (reset.Value() || drv_err || uv_cp)) {
    bool overtemp = false, prewarning = false;
    driver.status.GetTemperatureStatus(overtemp, prewarning);
    bool s2g_a = false, s2g_b = false;
    driver.status.IsShortToGround(s2g_a, s2g_b);
    bool ola = false, olb = false;
    driver.status.CheckOpenLoad(ola, olb);
    std::printf("reset=%d drv_err=%d uv_cp=%d ot=%d otpw=%d s2g=%d/%d ol=%d/%d\n", reset.Value(), drv_err,
                uv_cp, overtemp, prewarning, s2g_a, s2g_b, ola, olb);
    driver.status.ClearGlobalStatus();                               // write 1 to clear, after the fix
  }
  // --8<-- [end:errors]
}

void raw(Driver& driver) {
  // --8<-- [start:raw]
  auto& comm = driver.GetComm();
  const uint8_t where = driver.communication.GetDaisyChainPosition();   // SPI: chain position

  auto drv_status = comm.ReadRegister(tmc51x0::Registers::DRV_STATUS, where);
  if (drv_status) {
    std::printf("DRV_STATUS = 0x%08lX\n", static_cast<unsigned long>(drv_status.Value()));
  }
  comm.WriteRegister(tmc51x0::Registers::TPOWERDOWN, 10, where);         // write-only register
  // --8<-- [end:raw]
}

bool stallguard(Driver& driver) {
  // --8<-- [start:stallguard]
  driver.motorControl.SetStealthChopEnabled(false);       // StallGuard2 needs SpreadCycle

  tmc51x0::StallGuardConfig sg{};
  sg.threshold = 4;                                       // SGT, -64..63: higher = less sensitive
  sg.enable_filter = true;
  sg.min_velocity = 0.5f;                                 // TCOOLTHRS: ignore stalls below this speed
  sg.velocity_unit = Unit::RevPerSec;
  sg.stop_on_stall = true;                                // sg_stop in SW_MODE
  if (!driver.stallGuard.ConfigureStallGuard(sg)) {
    return false;
  }
  return driver.stallGuard.IsStallDetected().ValueOr(false);
  // --8<-- [end:stallguard]
}

void tune(Driver& driver) {
  // --8<-- [start:tune]
  tmc51x0::StallGuardTuningResult result{};
  auto r = driver.tuning.AutoTuneStallGuard(60.0f, result,          // target speed, in RPM below
                                            -20, 20,                 // SGT search range
                                            2.0f,                    // acceleration, rev/s²
                                            30.0f, 90.0f,            // speeds the SGT must also work at
                                            Unit::RPM, Unit::RevPerSec,
                                            0.3f);                   // 30 % current while tuning
  if (r && result.tuning_success) {
    tmc51x0::StallGuardConfig sg{};
    sg.threshold = result.optimal_sgt;
    driver.stallGuard.ConfigureStallGuard(sg);
  }
  // --8<-- [end:tune]
}

void home(Driver& driver) {
  // --8<-- [start:home]
  using Homing = Driver::Homing;

  Homing::BoundsOptions opt{};
  opt.speed_unit = Unit::RPM;
  opt.position_unit = Unit::Deg;
  opt.search_speed = 30.0f;          // RPM
  opt.search_span = 720.0f;          // at most two turns each way
  opt.backoff_distance = 5.0f;       // degrees off the hard stop
  opt.timeout_ms = 10000;

  Homing::HomeConfig home{};
  home.mode = Homing::HomePlacement::AtCenter;   // XACTUAL = 0 in the middle of the travel

  auto bounds = driver.homing.FindBounds(Homing::BoundsMethod::StallGuard, opt, home);
  if (bounds && bounds.Value().bounded) {
    std::printf("travel %.1f .. %.1f deg\n", bounds.Value().min_bound, bounds.Value().max_bound);
  }
  // --8<-- [end:home]
}

void encoder(Driver& driver) {
  // --8<-- [start:encoder]
  tmc51x0::EncoderConfig enc{};
  enc.pulses_per_rev = 4000;                         // counts per revolution after quadrature
  driver.encoder.Configure(enc);
  driver.encoder.SetResolution(200, 4000, false);    // motor full steps, encoder counts, inverted
  driver.encoder.SetAllowedDeviation(4);             // full steps before deviation_warn

  auto x_enc = driver.encoder.GetPosition();         // X_ENC
  if (driver.encoder.IsDeviationWarning().ValueOr(false)) {
    std::printf("step loss: encoder at %ld\n", static_cast<long>(x_enc.ValueOr(0)));
    driver.encoder.ClearDeviationWarning();
  }
  // --8<-- [end:encoder]
}

void six_point_ramp(Driver& driver) {
  // --8<-- [start:ramp]
  driver.rampControl.SetRampSpeeds(0.05f, 0.1f, 1.0f, Unit::RevPerSec);   // VSTART, VSTOP, V1
  driver.rampControl.SetFirstAcceleration(8.0f, Unit::RevPerSec);         // A1: VSTART .. V1
  driver.rampControl.SetAccelerations(4.0f, 4.0f, Unit::RevPerSec);       // AMAX, DMAX: V1 .. VMAX
  driver.rampControl.SetFinalDeceleration(8.0f, Unit::RevPerSec);         // D1: V1 .. VSTOP (never 0)
  driver.rampControl.SetMaxSpeed(3.0f, Unit::RevPerSec);                  // VMAX
  // --8<-- [end:ramp]
}

void millimetres(Driver& driver) {
  // --8<-- [start:mm]
  // With cfg.mechanical = LeadScrew, 8 mm pitch, given to Initialize():
  driver.rampControl.SetMaxSpeed(40.0f, Unit::Mm);            // mm/s: 5 rev/s
  driver.rampControl.MoveRelative(12.5f, Unit::Mm);           // 1.5625 rev
  const float where_mm = driver.rampControl.GetCurrentPosition(Unit::Mm).ValueOr(0.0f);
  std::printf("at %.2f mm\n", where_mm);
  // --8<-- [end:mm]
}
