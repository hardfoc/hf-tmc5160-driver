# StallGuard and homing

How the TMC5160 measures motor load without a sensor, how to tune the stall threshold, and how the driver uses it
to find the ends of travel and home the axis.

## StallGuard2

StallGuard2 compares the back-EMF to the commanded current. `SG_RESULT` in `DRV_STATUS` is 0 to 1023: high with
a light load, falling as the load rises, and 0 (or close to it) at a stall. The chip raises the stall flag when
the value reaches 0 for the current threshold `SGT`.

| Rule | Why |
| --- | --- |
| SpreadCycle only | StallGuard2 does not work in StealthChop. Disable StealthChop, or move above `TPWMTHRS` |
| Above `TCOOLTHRS` | The stall output and stop-on-stall are only active above this speed; below it the value is unreliable |
| Moderate speed | At very low speed the back-EMF is too small; at very high speed the motor runs out of voltage |
| A fixed current | `SG_RESULT` depends on the current; tune at the current you home with |

`SGT` runs from -64 to 63. Higher values make detection less sensitive; start at 0 and raise it when stalls are
reported without a load, lower it when real stalls are missed. The filter (`enable_filter`, `sfilt`) averages
four full steps for a steadier value at a quarter of the update rate.

```cpp
--8<-- "tests/docs/api_tasks.cpp:stallguard"
```

`stallGuard.GetStallGuardResult()` reads `SG_RESULT`; `IsStallDetected()` reads the latched stall event and
`ClearStallFlag()` clears it. With `stop_on_stall` the ramp generator stops the motor itself (`SW_MODE.sg_stop`).
The datasheet forbids combining it with soft stop (`SetSoftStop(true)`, deceleration on a switch event); the driver
returns `INVALID_STATE` if you try.

## Tune the threshold

The driver can search for `SGT` by running the motor at a target speed and testing thresholds:

```cpp
--8<-- "tests/docs/api_tasks.cpp:tune"
```

| Call | What it does |
| --- | --- |
| `tuning.TuneStallGuard()` | Finds the best `SGT` at the target speed, then checks it at the minimum and maximum speeds you give |
| `tuning.AutoTuneStallGuard()` | The same with the motor current reduced (30 % by default) for the duration, which makes stalls easier to detect and gentler on the mechanics |

The result reports `optimal_sgt`, whether it was found (`tuning_success`) and, when the minimum or maximum speed
does not work with it, the speeds that do. Run the tuning with the motor free to turn: it does not need a hard
stop. The [`stallguard_tuning`](examples.md) example runs it on hardware.

## Find bounds and home

`homing.FindBounds()` drives towards both ends of travel, records where it stopped, backs off and places the
home position:

```cpp
--8<-- "tests/docs/api_tasks.cpp:home"
```

| `BoundsMethod` | Detects an end by | Needs |
| --- | --- | --- |
| `StallGuard` | A stall against a hard stop | A tuned `SGT`; the routine switches to SpreadCycle and reduces the current (`current_reduction_factor`, 0.3 by default) |
| `Switch` | The left or right reference switch | Switches on REFL / REFR, configured with `switches` |
| `Encoder` | The encoder no longer following the motor | An ABN encoder configured with `encoder` |

| `HomePlacement` | Position 0 after homing |
| --- | --- |
| `None` | Unchanged |
| `AtMin`, `AtMax` | At the lower or upper end |
| `AtCenter` | In the middle of the travel |
| `AtOffsetFromMin` | `offset` (in `offset_unit`) above the lower end |

`BoundsOptions` sets the units, search speed, the most distance to try in each direction (`search_span`),
`backoff_distance`, a timeout per direction, and optional acceleration and StallGuard overrides. The routines save
StealthChop, currents and StallGuard settings first and restore them afterwards. Every routine takes an optional
cancel callback (`bool (*)()`), polled while the motor moves.

To home in one direction only, use `PerformSensorlessHoming()`, `PerformSwitchHoming()` or
`PerformEncoderIndexHoming()`; each returns the final position in microsteps.

!!! warning "Homing moves into hard stops"
    Sensorless homing deliberately stalls the motor. Use a reduced current and a low search speed, and check
    that your mechanics take the impact. Set `search_span` so a missed stall cannot drive the axis far past its
    travel.

Next: [Multi-chip](multi-chip.md).
