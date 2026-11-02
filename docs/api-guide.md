# API guide

How to do the common jobs with the driver: initialise, move, read, detect stalls, home, use an encoder and
handle faults.

Every snippet on this page is compiled by the host CI from
[`tests/docs/api_tasks.cpp`](../tests/docs/api_tasks.cpp), with `Driver` standing for `tmc51x0::TMC51x0<MySpi>`
and `Unit` for `tmc51x0::Unit`. The [API reference](api-reference.md) lists every member.

## The shape of the API

One `TMC51x0<Comm>` object per chip. Its public members are the chip's subsystems:

| Member | Use it to |
| --- | --- |
| `rampControl` | Set the ramp mode, targets, speeds and accelerations; read position and speed |
| `motorControl` | Enable and disable the bridges, set currents, chopper, StealthChop, CoolStep, DcStep |
| `thresholds` | Set the speeds at which StealthChop, CoolStep and high-speed mode switch |
| `powerStage` | Set gate drive and short protection (TMC5160) |
| `switches`, `events` | Configure reference switches, latching, position compare; clear ramp events |
| `encoder` | Configure and read an ABN encoder, detect step loss |
| `stallGuard`, `tuning`, `homing` | Detect stalls, tune the threshold, find the ends of travel |
| `status`, `io` | Read faults, temperature, load, input pins, chip version |
| `communication`, `uartConfig` | Daisy-chain position, UART node address, clock |
| `printer` | Print decoded registers during bring-up |

Every call that touches the bus returns `Result<T>`. It converts to `bool` (`true` = success), and has
`Value()`, `ValueOr(default)`, `Error()` and `ErrorMessage()`:

| `ErrorCode` | Meaning |
| --- | --- |
| `COMM_ERROR` | The port failed, a UART CRC was wrong, or a daisy-chain length was invalid |
| `NOT_INITIALIZED` | Called before `Initialize()` succeeded |
| `INVALID_VALUE` | A parameter is out of range (or a pin your port does not support) |
| `INVALID_STATE` | Not possible in the chip's mode, for example a ramp call with SD_MODE high |
| `TIMEOUT`, `CANCELLED` | A homing or tuning routine timed out or was cancelled |
| `HARDWARE_ERROR`, `SHORT_CIRCUIT`, `OPEN_LOAD`, `OVERTEMP_WARNING`, `OVERTEMP_SHUTDOWN` | A fault reported by the chip |
| `UNSUPPORTED` | The port or the chip variant does not have this feature |

## Initialise

```cpp
--8<-- "tests/docs/api_tasks.cpp:init"
```

`Initialize()` applies the whole `DriverConfig`: currents from `motor_spec`, power stage, chopper, StealthChop,
global configuration, ramp defaults, reference switches and encoder. Pass `verbose = false` to skip the
configuration table it logs. [Configuration](configuration.md) lists the fields. The destructor disables the
bridges.

## Move to a position

```cpp
--8<-- "tests/docs/api_tasks.cpp:position"
```

`SetTargetPosition()` starts the move as soon as `XTARGET` is written. `MoveRelative()` adds to the current
target. Poll `IsTargetReached()` (or watch `position_reached` in `SPI_STATUS`) for completion.
[Motion and units](motion-and-units.md) explains the ramp and the units.

## Run at a speed

```cpp
--8<-- "tests/docs/api_tasks.cpp:velocity"
```

In velocity mode the sign comes from the ramp mode, not from the speed. `Stop()` decelerates to zero.

## Read state

```cpp
--8<-- "tests/docs/api_tasks.cpp:read"
```

Each read is one register access (two SPI datagrams). `status` has decoded readings: `IsOvertempWarning()`,
`IsStealthChopActive()`, `GetActualCurrentScale()`, `GetLostSteps()` and more.

## Detect stalls

```cpp
--8<-- "tests/docs/api_tasks.cpp:stallguard"
```

```cpp
--8<-- "tests/docs/api_tasks.cpp:tune"
```

[StallGuard and homing](stallguard-and-homing.md) explains the rules StallGuard2 needs and what the tuning does.

## Home the axis

```cpp
--8<-- "tests/docs/api_tasks.cpp:home"
```

## Use an encoder

```cpp
--8<-- "tests/docs/api_tasks.cpp:encoder"
```

`SetResolution()` writes `ENC_CONST` so `X_ENC` counts in the same microsteps as `XACTUAL`; it uses a binary
prescaler when the ratio is exact and a decimal one otherwise. `SetAllowedDeviation()` takes full steps and arms
`deviation_warn` in `ENC_STATUS`.

## Handle errors and faults

```cpp
--8<-- "tests/docs/api_tasks.cpp:errors"
```

| Signal | Meaning | Where to look |
| --- | --- | --- |
| A call returns an error | The port failed, a CRC was wrong, or the chip is in the wrong mode | `ErrorMessage()`, your port's log |
| `GSTAT.reset` | The chip was reset (power-up, VS or VCC_IO undervoltage) | Run `Initialize()` again |
| `GSTAT.drv_err` | Overtemperature or short circuit switched the bridges off | `DRV_STATUS`: `ot`, `s2ga` / `s2gb`, `s2vsa` / `s2vsb` |
| `GSTAT.uv_cp` | Charge-pump undervoltage | VS too low, or a charge-pump capacitor missing |
| `DRV_STATUS.otpw` | Overtemperature pre-warning | Reduce current or improve cooling |
| `DRV_STATUS.ola` / `olb` | Open load | Motor wiring |

`GSTAT` is write-1-to-clear: clear it after you fixed the cause. A short-circuit shutdown stays latched until
the driver is disabled, by DRV_ENN (`motorControl.Disable()`) or by `TOFF` = 0. After an overtemperature shutdown
the bridges stay off until the chip has cooled below the pre-warning threshold. The open-load flags are
informative only and can trigger falsely in fast motion and at standstill: check them during slow motion.

## Access a register directly

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

[Communication protocols](protocols.md) has the register map.
