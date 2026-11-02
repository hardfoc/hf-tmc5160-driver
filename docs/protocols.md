# Communication protocols

What goes over the wire: the SPI datagram, the UART frames, and how the driver uses them to read and write
registers.

You need this page to write a port, to read a logic-analyser trace or to access a register the API does not
cover. The interface classes in [`inc/tmc51x0_comm_interface.hpp`](../inc/tmc51x0_comm_interface.hpp) implement
everything here.

## SPI datagram

Each access is 40 bits, MSB first, with CSN low for the whole datagram:

<div class="hf-registers" markdown>

| Bits | Host to chip | Chip to host |
| --- | --- | --- |
| `39` | W: 1 = write, 0 = read | `SPI_STATUS` bit 7 |
| `38..32` | Register address | `SPI_STATUS` bits 6..0 |
| `31..0` | Data (zero for a read) | Data latched by the previous datagram |

</div>

So a write of `VMAX` (0x27) = 0x00ABCDEF is the bytes `A7 00 AB CD EF`, and a read of `XACTUAL` (0x21) is
`21 00 00 00 00`. The data of a read comes back with the **next** datagram, so the driver sends the read twice and
takes the value from the second reply. After a write it sends a read of the same address and checks the reply,
which carries the value just written. Each register access therefore costs two datagrams.

## SPI_STATUS

The first byte of every SPI reply:

<div class="hf-registers" markdown>

| Bit | Name | Meaning |
| --- | --- | --- |
| `7` | `status_stop_r` | Right stop switch active (`RAMP_STAT[1]`) |
| `6` | `status_stop_l` | Left stop switch active (`RAMP_STAT[0]`) |
| `5` | `position_reached` | Target position reached (`RAMP_STAT[9]`) |
| `4` | `velocity_reached` | Target velocity reached (`RAMP_STAT[8]`) |
| `3` | `standstill` | Motor at standstill (`DRV_STATUS[31]`) |
| `2` | `sg2` | StallGuard2 flag (`DRV_STATUS[24]`) |
| `1` | `driver_error` | Driver error; clear `GSTAT` (`GSTAT[1]`) |
| `0` | `reset_flag` | The chip was reset; clear `GSTAT` (`GSTAT[0]`) |

</div>

`tmc51x0::SpiStatus` decodes it (`PositionReached()`, `DriverError()`, ...). The driver logs `driver_error`
(rate-limited) but does not fail the access because of it: read `GSTAT` and `DRV_STATUS` to find the cause.

## UART frames

Every byte is 8N1, LSB first; multi-byte data is sent high byte first. Each frame starts with the sync nibble
(`0x05` in the first byte) and ends with a CRC8.

| Frame | Bytes | Layout |
| --- | --- | --- |
| Write | 8 | `05`, node address, register address \| 0x80, 4 data bytes, CRC |
| Read request | 4 | `05`, node address, register address, CRC |
| Read reply | 8 | `05`, `FF` (master address), register address, 4 data bytes, CRC |

The CRC is CRC8 with polynomial x⁸ + x² + x + 1 over all preceding bytes, each byte taken LSB first, as in the
datasheet's reference code. A write gets no reply; the chip counts accepted writes in `IFCNT`. The driver checks
the CRC of every reply and returns `COMM_ERROR` on a mismatch.

On a single wire the MCU receives its own transmission. Your port must discard that echo, so `UartReceive()`
returns only the chip's reply ([Porting](porting.md#uart-port)).

## Register map

| Range | Group | Examples |
| --- | --- | --- |
| `0x00`–`0x0F` | General configuration | `GCONF`, `GSTAT`, `IFCNT`, `NODECONF`, `IOIN` / `OUTPUT`, `DRV_CONF`, `GLOBAL_SCALER` |
| `0x10`–`0x15` | Velocity-dependent control | `IHOLD_IRUN`, `TPOWERDOWN`, `TSTEP`, `TPWMTHRS`, `TCOOLTHRS`, `THIGH` |
| `0x20`–`0x2D` | Ramp generator | `RAMPMODE`, `XACTUAL`, `VACTUAL`, `VSTART`, `A1`, `V1`, `AMAX`, `VMAX`, `DMAX`, `D1`, `VSTOP`, `XTARGET` |
| `0x33`–`0x36` | Ramp generator features | `VDCMIN`, `SW_MODE`, `RAMP_STAT`, `XLATCH` |
| `0x38`–`0x3D` | Encoder | `ENCMODE`, `X_ENC`, `ENC_CONST`, `ENC_STATUS`, `ENC_LATCH`, `ENC_DEVIATION` |
| `0x60`–`0x73` | Motor driver | `MSLUT0..7`, `CHOPCONF`, `COOLCONF`, `DCCTRL`, `DRV_STATUS`, `PWMCONF`, `PWM_SCALE`, `LOST_STEPS` |

Most registers are write-only; the driver keeps a copy of what it wrote. `GSTAT`, `RAMP_STAT` and `ENC_STATUS`
are cleared by writing 1 to a bit. The addresses are constants in `tmc51x0::Registers` and the bit fields are
unions such as `CHOPCONF_Register` in [`inc/registers/`](../inc/registers/tmc51x0_registers.hpp).

## Raw register access

```cpp
--8<-- "tests/docs/api_tasks.cpp:raw"
```

`GetComm()` returns your port; `ReadRegister()` and `WriteRegister()` take the chip's daisy-chain position (SPI)
or node address (UART) as the last argument. Writing a register behind the driver's back makes its copy stale:
prefer the subsystem calls for anything they cover.

Next: [TMC5130](tmc5130.md).
