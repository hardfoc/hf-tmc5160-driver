# Motion and units

How the TMC5160's internal ramp generator moves the motor, and how the driver converts degrees, millimetres and
RPM into register values.

## The ramp generator

With SD_MODE low the chip generates the step pulses itself. You write a target and a speed profile; it
accelerates, cruises and decelerates on its own, and the host only polls for completion. All motion parameters
can be changed while the motor runs.

| Ramp mode | `RampMode` | Behaviour |
| --- | --- | --- |
| Positioning | `POSITIONING` | Moves to `XTARGET` with the full six-segment profile and stops there. `Initialize()` selects it |
| Velocity, positive / negative | `VELOCITY_POS`, `VELOCITY_NEG` | Accelerates with `AMAX` to `VMAX` and keeps running |
| Hold | `HOLD` | Keeps the current velocity unless a stop event occurs |

In positioning mode the chip uses a SixPoint ramp:

| Segment | From → to | Rate | Driver call |
| --- | --- | --- | --- |
| Start | 0 → `VSTART` | jump | `SetRampSpeeds()` |
| First acceleration | `VSTART` → `V1` | `A1` | `SetFirstAcceleration()` |
| Main acceleration | `V1` → `VMAX` | `AMAX` | `SetAcceleration()`, `SetAccelerations()` |
| Main deceleration | `VMAX` → `V1` | `DMAX` | `SetDeceleration()`, `SetAccelerations()` |
| Final deceleration | `V1` → `VSTOP` | `D1` | `SetFinalDeceleration()` |
| Stop | `VSTOP` → 0 | jump | `SetRampSpeeds()` |

With `V1` = 0 the ramp is a plain trapezoid using `AMAX` and `DMAX`. `D1` must not be 0 in positioning mode, even
when `V1` is 0, and `VSTOP` must be at least `VSTART`.

```cpp
--8<-- "tests/docs/api_tasks.cpp:ramp"
```

After a stop the chip waits `TZEROWAIT` before reversing (`SetZeroWaitTimeMs()`), and after `TPOWERDOWN` at
standstill it lowers the current from `IRUN` to `IHOLD` (`SetPowerDownDelayMs()`).

## Units

Every position, speed and acceleration call takes a `Unit`:

| `Unit` | Position | Speed | Acceleration |
| --- | --- | --- | --- |
| `Steps` | full steps | full steps/s | full steps/s² |
| `Deg`, `Rad` | of the output shaft | per second | per second² |
| `RevPerSec` | n/a | rev/s (the recommended default) | rev/s² |
| `RPM` | n/a | rev/min | n/a |
| `Mm` | linear, needs a lead screw or belt | mm/s | mm/s² |

The register values are microsteps: one full step is 2<sup>MRES</sup> microsteps (256 by default). The driver
converts with the formulas of the datasheet (section 12.1):

| Quantity | Register to physical |
| --- | --- |
| Velocity | v[µsteps/s] = `VMAX` × f<sub>CLK</sub> / 2<sup>24</sup> |
| Acceleration | a[µsteps/s²] = `AMAX` × f<sub>CLK</sub>² / (512 × 256) / 2<sup>24</sup> |
| Revolutions | v[rev/s] = v[µsteps/s] / microsteps per full step / full steps per rev |

At the internal 12 MHz clock, 1 rev/s of a 200-step motor at 256 microsteps is 51 200 µsteps/s, a `VMAX` of
about 71 600. The conversions use `DriverConfig::external_clk_config.frequency_hz` (0 means the internal
12 MHz), `motor_spec.steps_per_rev` and the current microstep resolution, so a change of `MRES` keeps the physical
speed. The internal oscillator is accurate to about ± 4 %; feed an external clock where speed accuracy matters.

## Mechanical systems

`DriverConfig::mechanical` turns motor revolutions into output motion.

| `MechanicalSystemType` | Fields | Effect |
| --- | --- | --- |
| `DirectDrive` | none | Degrees and revolutions are the motor shaft |
| `Gearbox` | `gear_ratio` (output / input) | Degrees and revolutions are the output shaft |
| `LeadScrew` | `lead_screw_pitch_mm` | `Unit::Mm`: one revolution = pitch |
| `BeltDrive` | `belt_pulley_teeth`, `belt_pitch_mm` | `Unit::Mm`: one revolution = teeth × pitch |

```cpp
--8<-- "tests/docs/api_tasks.cpp:mm"
```

!!! warning "Millimetres need a mechanical system"
    With `DirectDrive` or `Gearbox`, or a zero pitch, the `Unit::Mm` conversions return 0: a `SetTargetPosition(x,
    Unit::Mm)` then becomes a move to position 0. Configure `mechanical` before you use millimetres.

## Position readback

`GetCurrentPosition()` reads `XACTUAL`; `GetTargetPosition()` reads `XTARGET`. `SetCurrentPosition()` redefines
where you are without moving (for homing), and can update the encoder count with it. The raw microstep values
are available from `GetCurrentPositionMicrosteps()` and `GetTargetPositionMicrosteps()`.

Next: [Current and chopper](current-and-chopper.md).
