# Current and chopper

How the driver sets the coil current from the motor's datasheet, and how to choose between SpreadCycle,
StealthChop, CoolStep and DcStep.

## Coil current

Three settings multiply to the coil current:

I<sub>RMS</sub> = (`GLOBAL_SCALER` / 256) × ((CS + 1) / 32) × V<sub>FS</sub> / R<sub>SENSE</sub> / √2

with CS = `IRUN` while moving and `IHOLD` at standstill, and V<sub>FS</sub> = 325 mV typical. `GLOBAL_SCALER` is
32 to 255 (0 means 256; 1 to 31 are not allowed), `IRUN` and `IHOLD` are 0 to 31.

You do not compute these. Fill `DriverConfig::motor_spec` and `Initialize()` (or
`motorControl.ConfigureMotorCurrent()`) does it:

| `MotorSpec` field | Meaning | Default |
| --- | --- | --- |
| `rated_current_ma` | Motor rated current, RMS | 1500 |
| `run_current_ma` | Run current; 0 uses the rated current | 0 |
| `hold_current_ma` | Standstill current; 0 uses 30 % of the run current | 0 |
| `sense_resistor_mohm` | R<sub>SENSE</sub> on your board | 50 |
| `supply_voltage_mv` | VS | 24000 |
| `steps_per_rev` | Full steps per revolution | 200 |
| `winding_resistance_mohm`, `winding_inductance_mh` | Used for the StealthChop limits | 3, 0 |
| `iholddelay_ms` | Time to ramp from run to hold current at standstill | 100 |
| `scaler_adjustment_percent`, `irun_adjustment_percent`, `ihold_adjustment_percent` | Trim the calculated values | 0 |

The calculation starts at `IRUN` = 31 and solves for `GLOBAL_SCALER`; when the scaler would exceed 200 it lowers
`IRUN` (not below 16) to keep the scaler in range. A run current more than 10 % above the full-scale current of
the sense resistor fails. With 50 mΩ and 1500 mA the result is `IRUN` = 31, `GLOBAL_SCALER` ≈ 84 (derived).
`LogDerivedInitSummary()` prints what was written; `motorControl.SetCurrent(irun, ihold)` and
`SetGlobalScaler()` set the registers directly.

!!! note "TMC5130"
    The TMC5130 has no `GLOBAL_SCALER`. On a TMC5130 the driver never writes it and calculates `IRUN` / `IHOLD`
    from the TMC5130's own formula ([TMC5130](tmc5130.md#current-on-the-tmc5130)).

## Choosing a chopper

| Mode | What it does | Choose it for | Limits |
| --- | --- | --- | --- |
| **StealthChop2** | Voltage-mode PWM, near silent | Low and medium speed, standstill | No StallGuard2; the first motion after power-up calibrates it (`IRUN` ≥ 8) |
| **SpreadCycle** | Cycle-by-cycle current control | High speed, high dynamics, any load | Audible chopper noise at low speed |
| **CoolStep** | Lowers the current while StallGuard2 reports a light load | Saving energy and heat | Needs SpreadCycle above `TCOOLTHRS` |
| **DcStep** | Lets the motor slow down under overload instead of losing steps | Variable loads near the torque limit | Above `VDCMIN`; set it up with the datasheet |

`DriverConfig::global_config.en_stealthchop_mode` (on by default) selects StealthChop. The chip then switches by
speed, measured as `TSTEP` (the time between microsteps):

| Threshold | Register | Above this speed |
| --- | --- | --- |
| StealthChop to SpreadCycle | `TPWMTHRS` | SpreadCycle |
| CoolStep and StallGuard2 active | `TCOOLTHRS` | CoolStep and the stall output work |
| High-speed mode | `THIGH` | Full step (`vhighfs`) or classic chopper (`vhighchm`) |

Set the three at once in physical units with `thresholds.SetModeChangeSpeeds()`, or one at a time with
`SetStealthChopVelocityThreshold()`, `SetTcoolthrs()` and `SetHighSpeedThreshold()`.

```cpp
--8<-- "tests/docs/configuration.cpp:runtime"
```

## SpreadCycle timing

`ChopperConfig` holds the `CHOPCONF` fields. The driver's defaults (`TOFF` = 5, `TBL` = 2, `HSTRT` = 4,
`HEND` = 0) work for most motors; tune `HSTRT` and `HEND` with the datasheet's procedure (chapter 7) when the
motor is noisy or the current waveform is distorted. `mres` sets the microstep resolution (256 down to full step)
and `intpol` interpolates lower resolutions to 256 microsteps.

## StealthChop PWM frequency

`StealthChopConfig::pwm_freq` selects f<sub>PWM</sub> as a fraction of f<sub>CLK</sub>:

| `pwm_freq` | f<sub>PWM</sub> | At 12 MHz |
| --- | --- | --- |
| 0 | 2/1024 f<sub>CLK</sub> | 23.4 kHz |
| 1 (default) | 2/683 f<sub>CLK</sub> | 35.1 kHz |
| 2 | 2/512 f<sub>CLK</sub> | 46.9 kHz |
| 3 | 2/410 f<sub>CLK</sub> | 58.5 kHz |

Automatic tuning (`pwm_autoscale`, `pwm_autograd`) needs a run current of `IRUN` ≥ 8 and a first move at
moderate speed. Changing `GLOBAL_SCALER` or VS afterwards invalidates the tuning until the chip re-tunes.

## Power stage (TMC5160)

The TMC5160 drives external MOSFETs. `PowerStageParameters` sets the gate drive from MOSFET data:

| Field | Sets | Default |
| --- | --- | --- |
| `mosfet_miller_charge_nc` | `DRVSTRENGTH` (gate current 200 mA to 600 mA low side) | 10 nC |
| `bbm_time_ns` | Break-before-make time, `BBMTIME` / `BBMCLKS` | 100 ns |
| `sense_filter` | `FILT_ISENSE` | 100 ns |
| `over_temp_protection` | `OTSELECT` | 150 °C |
| `s2vs_voltage_mv`, `s2g_voltage_mv`, `shortfilter`, `short_detection_delay_us_x10` | `SHORT_CONF` | 0 = chip default |

Next: [StallGuard and homing](stallguard-and-homing.md).
