/**
 * @file test_driver.cpp
 * @brief The driver over the SPI chip model: register access, initialisation, chip detection, motion commands.
 */
#include "test_harness.hpp"

#include "mock_tmc51x0.hpp"

using Driver = tmc51x0::TMC51x0<MockSpi>;
namespace reg = tmc51x0::Registers;

TEST_CASE("ReadRegister pipelines: two datagrams, value from the second reply") {
  MockSpi spi;
  spi.reg(reg::XACTUAL) = 0x00012345;
  const int before = spi.transfers;
  auto r = spi.ReadRegister(reg::XACTUAL);
  REQUIRE(r.IsOk());
  CHECK_EQ(r.Value(), 0x00012345u);
  CHECK_EQ(spi.transfers - before, 2);
  CHECK_EQ(spi.lengths.back(), 5u);
}

TEST_CASE("WriteRegister: the write datagram, then a read of the same address for the confirmation") {
  MockSpi spi;
  const int before = spi.transfers;
  REQUIRE(spi.WriteRegister(reg::VMAX, 51200).IsOk());
  CHECK_EQ(spi.transfers - before, 2);
  CHECK_EQ(spi.reg(reg::VMAX), 51200u);
  REQUIRE(!spi.writes.empty());
  CHECK_EQ(spi.writes.back().first, reg::VMAX);
  CHECK_EQ(spi.writes.back().second, 51200u);
}

TEST_CASE("A failing transfer surfaces as COMM_ERROR") {
  MockSpi spi;
  // A zero-length transfer is rejected by the model, as a broken bus would be.
  uint8_t tx[1]{};
  uint8_t rx[1]{};
  CHECK(spi.SpiTransfer(tx, rx, 0).Error() == tmc51x0::ErrorCode::COMM_ERROR);
}

TEST_CASE("Result<T> carries a value or an error") {
  tmc51x0::Result<int> ok(42);
  tmc51x0::Result<int> err(tmc51x0::ErrorCode::TIMEOUT);
  CHECK(static_cast<bool>(ok));
  CHECK_EQ(ok.Value(), 42);
  CHECK(err.IsErr());
  CHECK(err.Error() == tmc51x0::ErrorCode::TIMEOUT);
  CHECK_EQ(err.ValueOr(-1), -1);
  auto [code, value] = ok;
  CHECK(code == tmc51x0::ErrorCode::OK);
  CHECK_EQ(value, 42);
}

TEST_CASE("Initialize detects a TMC5160 and writes GLOBAL_SCALER") {
  MockSpi spi(tmc51x0::ChipVersion::TMC5160);
  Driver driver(spi);
  tmc51x0::DriverConfig cfg{};
  cfg.motor_spec.rated_current_ma = 1500;
  cfg.motor_spec.sense_resistor_mohm = 50;
  auto r = driver.Initialize(cfg, false);
  REQUIRE(r.IsOk());
  CHECK_EQ(driver.status.GetChipVersion(), tmc51x0::ChipVersion::TMC5160);
  uint32_t scaler = 0;
  CHECK(spi.lastWrite(reg::GLOBAL_SCALER, scaler));
  uint32_t ihold_irun = 0;
  CHECK(spi.lastWrite(reg::IHOLD_IRUN, ihold_irun));
}

TEST_CASE("Initialize detects a TMC5130; SetGlobalScaler then writes nothing") {
  MockSpi spi(tmc51x0::ChipVersion::TMC5130);
  Driver driver(spi);
  REQUIRE(driver.Initialize(tmc51x0::DriverConfig{}, false).IsOk());
  CHECK_EQ(driver.status.GetChipVersion(), tmc51x0::ChipVersion::TMC5130);
  spi.writes.clear();
  REQUIRE(driver.motorControl.SetGlobalScaler(128).IsOk());
  uint32_t scaler = 0;
  CHECK(!spi.lastWrite(reg::GLOBAL_SCALER, scaler));
}

TEST_CASE("TMC5130: Initialize never writes the TMC5160-only registers 0x09..0x0B") {
  // TMC5160A datasheet section 6.1: SHORT_CONF (0x09), DRV_CONF (0x0A) and GLOBAL_SCALER (0x0B) are TMC5160 only.
  MockSpi spi(tmc51x0::ChipVersion::TMC5130);
  Driver driver(spi);
  tmc51x0::DriverConfig cfg{};
  cfg.motor_spec.run_current_ma = 1000;
  cfg.motor_spec.sense_resistor_mohm = 50;
  REQUIRE(driver.Initialize(cfg, false).IsOk());
  uint32_t v = 0;
  CHECK(!spi.lastWrite(reg::GLOBAL_SCALER, v));
  CHECK(!spi.lastWrite(reg::DRV_CONF, v));
  CHECK(!spi.lastWrite(reg::SHORT_CONF, v));
}

TEST_CASE("TMC5130: IRUN follows the TMC5130 current formula, not GLOBAL_SCALER") {
  // TMC5130A: I_RMS = ((CS+1)/32) * 0.32 V / (RSENSE + 20 mOhm) / sqrt(2). With 50 mOhm the full scale is
  // 3.23 A, so 1.0 A needs CS = 8 (0.91 A; rounded down). The TMC5160 formula would give IRUN = 31 (3.2 A).
  MockSpi spi(tmc51x0::ChipVersion::TMC5130);
  Driver driver(spi);
  tmc51x0::DriverConfig cfg{};
  cfg.motor_spec.run_current_ma = 1000;
  cfg.motor_spec.hold_current_ma = 500;
  cfg.motor_spec.sense_resistor_mohm = 50;
  REQUIRE(driver.Initialize(cfg, false).IsOk());
  uint32_t ihold_irun = 0;
  REQUIRE(spi.lastWrite(reg::IHOLD_IRUN, ihold_irun));
  CHECK_EQ((ihold_irun >> 8) & 0x1Fu, 8u);
  CHECK_EQ(ihold_irun & 0x1Fu, 3u);
}

TEST_CASE("TMC5130: VerifySetup accepts version 0x11 without a mismatch warning") {
  MockSpi spi(tmc51x0::ChipVersion::TMC5130);
  Driver driver(spi);
  REQUIRE(driver.Initialize(tmc51x0::DriverConfig{}, false).IsOk());
  spi.warnings.clear();
  (void)driver.status.VerifySetup();
  bool version_warning = false;
  for (const auto& tag : spi.warnings) {
    version_warning = version_warning || tag == "VerifySetup";
  }
  CHECK(!version_warning);
}

TEST_CASE("CalculateMotorCurrent keeps the TMC5160 result by default") {
  tmc51x0::MotorSpec spec{};
  uint8_t irun = 0;
  uint8_t ihold = 0;
  uint16_t scaler = 0;
  REQUIRE(tmc51x0::CalculateMotorCurrent(spec, 50, 24000, 1500, 0, irun, ihold, scaler));
  CHECK_EQ(irun, 31u);
  CHECK(scaler >= 80u && scaler <= 86u);
}

TEST_CASE("Motion commands write RAMPMODE, VMAX and XTARGET in microsteps") {
  MockSpi spi;
  Driver driver(spi);
  REQUIRE(driver.Initialize(tmc51x0::DriverConfig{}, false).IsOk());
  REQUIRE(driver.rampControl.SetRampMode(tmc51x0::RampMode::POSITIONING).IsOk());
  REQUIRE(driver.rampControl.SetMaxSpeed(60.0f, tmc51x0::Unit::RPM).IsOk());
  REQUIRE(driver.rampControl.SetTargetPosition(10.0f, tmc51x0::Unit::Steps).IsOk());
  CHECK_EQ(spi.reg(reg::RAMPMODE), 0u);
  CHECK(spi.reg(reg::VMAX) > 0u);
  CHECK_EQ(spi.reg(reg::XTARGET), 2560u);  // full steps x 256 microsteps
}

TEST_CASE("Enable drives EN (DRV_ENN) to ACTIVE through the port") {
  MockSpi spi;
  Driver driver(spi);
  REQUIRE(driver.Initialize(tmc51x0::DriverConfig{}, false).IsOk());
  spi.gpio_sets.clear();
  REQUIRE(driver.motorControl.Enable().IsOk());
  bool saw_enable = false;
  for (const auto& s : spi.gpio_sets) {
    if (s.first == tmc51x0::TMC51x0CtrlPin::EN && s.second == tmc51x0::GpioSignal::ACTIVE) {
      saw_enable = true;
    }
  }
  CHECK(saw_enable);
}

TEST_CASE("Unit::Mm without a lead screw or belt is rejected, not turned into a move to 0") {
  MockSpi spi;
  Driver driver(spi);
  REQUIRE(driver.Initialize(tmc51x0::DriverConfig{}, false).IsOk());  // mechanical: direct drive
  spi.reg(reg::XTARGET) = 12345;
  auto r = driver.rampControl.SetTargetPosition(10.0f, tmc51x0::Unit::Mm);
  CHECK(r.Error() == tmc51x0::ErrorCode::INVALID_VALUE);
  CHECK_EQ(spi.reg(reg::XTARGET), 12345u);
  CHECK(driver.rampControl.SetMaxSpeed(5.0f, tmc51x0::Unit::Mm).Error() == tmc51x0::ErrorCode::INVALID_VALUE);
  CHECK(driver.rampControl.MoveRelative(1.0f, tmc51x0::Unit::Mm).Error() == tmc51x0::ErrorCode::INVALID_VALUE);
}

TEST_CASE("Unit::Mm with a lead screw converts: 8 mm pitch, 4 mm = half a turn") {
  MockSpi spi;
  Driver driver(spi);
  tmc51x0::DriverConfig cfg{};
  cfg.mechanical.system_type = tmc51x0::MechanicalSystemType::LeadScrew;
  cfg.mechanical.lead_screw_pitch_mm = 8.0f;
  REQUIRE(driver.Initialize(cfg, false).IsOk());
  REQUIRE(driver.rampControl.SetTargetPosition(4.0f, tmc51x0::Unit::Mm).IsOk());
  CHECK_EQ(spi.reg(reg::XTARGET), 25600u);  // 200 full steps x 256 / 2
}

TEST_CASE("Degrees convert to microsteps: 180 deg = 100 full steps x 256") {
  MockSpi spi;
  Driver driver(spi);
  REQUIRE(driver.Initialize(tmc51x0::DriverConfig{}, false).IsOk());
  REQUIRE(driver.rampControl.SetTargetPosition(180.0f, tmc51x0::Unit::Deg).IsOk());
  CHECK_EQ(spi.reg(reg::XTARGET), 25600u);
}

