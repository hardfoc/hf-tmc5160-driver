# Configuration

Every setting `Initialize()` applies, how to build a configuration, and what to change at run time.

## DriverConfig

`tmc51x0::DriverConfig` (in [`inc/tmc51x0_types.hpp`](../inc/tmc51x0_types.hpp)) holds the whole chip setup.
Every member has a safe default; a minimal configuration sets only the motor and board values.

| Member | Type | Sets | Page |
| --- | --- | --- | --- |
| `motor_spec` | `MotorSpec` | Currents (`GLOBAL_SCALER`, `IRUN`, `IHOLD`, `IHOLDDELAY`), steps per revolution | [Current and chopper](current-and-chopper.md) |
| `power_stage` | `PowerStageParameters` | `DRV_CONF`, `SHORT_CONF` (gate drive, BBM, short detection) | [Current and chopper](current-and-chopper.md#power-stage-tmc5160) |
| `mechanical` | `MechanicalSystem` | Gearbox, lead screw or belt for unit conversion | [Motion and units](motion-and-units.md#mechanical-systems) |
| `direction` | `MotorDirection` | Normal or inverted motor direction | |
| `chopper` | `ChopperConfig` | `CHOPCONF`: SpreadCycle timing, microsteps, interpolation | [Current and chopper](current-and-chopper.md#spreadcycle-timing) |
| `stealthchop` | `StealthChopConfig` | `PWMCONF`, `TPWMTHRS` | [Current and chopper](current-and-chopper.md#stealthchop-pwm-frequency) |
| `global_config` | `GlobalConfig` | `GCONF`: StealthChop enable, DIAG outputs, direct mode | |
| `ramp_config` | `RampConfig` | `VSTART`, `VSTOP`, `V1`, `VMAX`, `A1`, `AMAX`, `DMAX`, `D1`, `TPOWERDOWN`, `TZEROWAIT` | [Motion and units](motion-and-units.md) |
| `external_clk_config` | `ExternalClockConfig` | f<sub>CLK</sub> for all conversions; 0 = internal 12 MHz | [Hardware](hardware.md#power) |
| `stallguard` | `StallGuardConfig` | `SGT`, filter, `TCOOLTHRS`, stop on stall | [StallGuard and homing](stallguard-and-homing.md) |
| `coolstep` | `CoolStepConfig` | `COOLCONF`; disabled while `lower_threshold_sg` is 0 | [Current and chopper](current-and-chopper.md#choosing-a-chopper) |
| `dcstep` | `DcStepConfig` | `DCCTRL`, `VDCMIN`; disabled by default | |
| `reference_switch_config` | `ReferenceSwitchConfig` | `SW_MODE`: switch polarity, stop, latch | |
| `encoder_config` | `EncoderConfig` | `ENCMODE`, resolution, deviation | [API guide](api-guide.md#use-an-encoder) |
| `uart_config` | `UartConfig` | `NODECONF` (UART only) | [Multi-chip](multi-chip.md#uart-nodes) |

`GetDriverConfig()` returns the configuration with every change made since `Initialize()`.

## Fill the struct

```cpp
--8<-- "tests/docs/configuration.cpp:struct"
```

## Or use the builder

`tmc51x0::ConfigBuilder` (in [`inc/features/tmc51x0_config_builder.hpp`](../inc/features/tmc51x0_config_builder.hpp))
sets the same fields with chained calls and returns a `DriverConfig`:

```cpp
--8<-- "tests/docs/configuration.cpp:builder"
```

## Change settings at run time

Each subsystem changes its part of the configuration while the motor runs:

```cpp
--8<-- "tests/docs/configuration.cpp:runtime"
```

| Change | Call |
| --- | --- |
| Currents | `motorControl.SetCurrent()`, `ConfigureMotorCurrent()`, `SetGlobalScaler()`, `SetIholdDelayMs()` |
| Chopper and microsteps | `motorControl.ConfigureChopper()`, `SetMicrostepResolution()`, `ConfigureStealthChop()`, `SetStealthChopEnabled()` |
| Mode-change speeds | `thresholds.SetModeChangeSpeeds()`, `SetModeHysteresis()` |
| Ramp | `rampControl.ConfigureRamp()` or the individual setters |
| Protection | `powerStage.ConfigurePowerStage()`, `ConfigureShortProtection()` |

## Starting points

| Goal | Settings |
| --- | --- |
| Quiet at low speed | StealthChop on (`global_config.en_stealthchop_mode`), `pwm_autoscale` and `pwm_autograd` on, `TPWMTHRS` at the highest speed you need quiet |
| Torque and speed | StealthChop off, SpreadCycle with the default timing, `THIGH` with `vhighfs` for full step at high speed |
| Sensorless homing | SpreadCycle during homing, `TCOOLTHRS` below the homing speed, a tuned `SGT` |
| Step-loss detection | An ABN encoder, `SetResolution()` and `SetAllowedDeviation()` |

## The ESP32 examples' configuration

The examples split their settings into board, motor and platform structs in
[`esp32_tmc51x0_test_config.hpp`](../examples/esp32/main/test_config/esp32_tmc51x0_test_config.hpp):
`BoardConfig_TMC51x0_EVAL` (50 mΩ sense resistors) and `BoardConfig_TMC51x0_BOB` (110 mΩ), motor structs such as
`MotorConfig_17HS4401S`, and platform structs for switches, encoder and mechanics.
`ConfigureDriverFromTestRig()` combines them into a `DriverConfig`.

Compile-time switches are on the [Installation](installation.md#compile-time-switches) page.
