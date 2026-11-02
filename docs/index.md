---
hide:
  - navigation
  - toc
---

<div class="hf-hero" markdown>
<span class="hf-kicker">HardFOC driver</span>

# TMC5160 Driver

<p>A C++20 driver for the Analog Devices (Trinamic) TMC5160 and TMC5130 stepper motor controllers. It sets the
motor current from the motor's datasheet values, runs the chip's internal ramp generator in degrees, millimetres
or RPM, and gives you StallGuard2, CoolStep, StealthChop, encoder feedback and sensorless homing, on any MCU with
SPI or a UART.</p>

[Quickstart](quickstart.md){ .md-button .md-button--primary }
[API guide](api-guide.md){ .md-button }
[Source on GitHub](https://github.com/hardfoc/hf-tmc5160-driver){ .md-button }
</div>

The driver has no platform code. You give it a small class that moves bytes over SPI or a UART, sets the
DRV_ENN pin and waits; it does the rest: datagram framing, daisy-chain padding, UART CRC, register caching,
current calculation, unit conversion and fault decoding.

<div class="hf-chip" markdown>
<div class="hf-chip__head">
<span class="hf-chip__kicker">Stepper controller and gate driver with motion controller</span>
<span class="hf-chip__name">TMC5160</span>
<span class="hf-chip__vendor">Analog Devices (Trinamic)</span>
</div>

Function
:   2-phase bipolar stepper, external MOSFET bridges for 1 A to several 10 A per coil, up to 256 microsteps

Motion
:   Internal SixPoint ramp generator (positioning and velocity modes), or STEP/DIR input

Host interface
:   SPI mode 3, 40-bit datagrams, up to 4 MHz on the internal clock; or single-wire UART with CRC8

Supply
:   VS 10 V to 55 V operating (8 V reduced spec, 60 V absolute maximum); VCC_IO 3.0 V to 5.25 V

Features
:   StallGuard2, CoolStep, StealthChop2, SpreadCycle, DcStep, ABN encoder, two reference switches

Package
:   eTQFP48 (7 x 7 mm body) or QFN56 (8 x 8 mm)

Also supported
:   TMC5130: the same register set with integrated MOSFETs (up to 1.4 A), detected at start-up

[TMC5160 datasheet](datasheet/tmc5160a-datasheet-rev1.18.pdf) ·
[TMC5130 differences](tmc5130.md)
</div>

## What you get

- **One object per chip.** `TMC51x0<YourPort>` exposes the chip as named subsystems: `rampControl`,
  `motorControl`, `stallGuard`, `encoder`, `homing`, `status` and more. See [API guide](api-guide.md).
- **Current from the motor datasheet.** Give rated current, sense resistor and supply; the driver computes
  `GLOBAL_SCALER`, `IRUN` and `IHOLD`. See [Current and chopper](current-and-chopper.md).
- **Physical units.** Positions in degrees, steps or millimetres, speeds in RPM or rev/s, with lead screw, belt
  and gearbox ratios. See [Motion and units](motion-and-units.md).
- **Sensorless homing.** StallGuard2 threshold tuning and bounds finding without end switches. See
  [StallGuard and homing](stallguard-and-homing.md).
- **Several chips on one bus.** SPI daisy chains and UART node addresses. See [Multi-chip](multi-chip.md).
- **Zero-overhead portability.** The port is a CRTP class: no virtual calls, no RTOS dependency, every call
  returns `Result<T>`. See [Porting](porting.md).
- **Tested without hardware.** Host unit tests run the driver against a model of the chip's SPI interface. See
  [Testing and CI](testing.md).

## Where to go next

<div class="grid cards" markdown>

-   :material-rocket-launch-outline: **[Quickstart](quickstart.md)**

    ---

    Wire the chip, write the port, turn the motor half a revolution.

-   :material-chip: **[Hardware](hardware.md)**

    ---

    Pins, mode straps, bus settings, power and the sense resistor.

-   :material-layers-outline: **[Architecture](architecture.md)**

    ---

    The three layers and what each one owns.

-   :material-code-braces: **[API guide](api-guide.md)**

    ---

    Initialise, move, read, detect stalls and handle faults, task by task.

-   :material-swap-horizontal: **[Porting](porting.md)**

    ---

    Implement the communication interface for your MCU.

-   :material-book-open-variant: **[API reference](api-reference.md)**

    ---

    Every class and member, generated from the headers.

</div>
