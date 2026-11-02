"""The three layers of the driver: hardware-agnostic core, communication interface, platform port.

Reads: From top to bottom. Your application talks to one TMC51x0<Comm> object per chip, whose public members are
the chip's subsystems (listed from inc/tmc51x0.hpp). Each subsystem turns a call in physical units into register
reads and writes. The communication interface (SpiCommInterface or UartCommInterface in
inc/tmc51x0_comm_interface.hpp) builds the 40-bit SPI datagrams or CRC-checked UART frames, handles daisy-chain
padding and node addresses. The platform port is the only code you write: the methods the interface calls on it,
read from the header. Below it is the TMC5160 or TMC5130.
"""

import re
from pathlib import Path

from hardfoc_mkdocs_theme.figkit import Fig

ROOT = Path(__file__).resolve().parents[3]
DRIVER = "inc/tmc51x0.hpp"
COMM = "inc/tmc51x0_comm_interface.hpp"


def data():
    drv = (ROOT / DRIVER).read_text()
    subsystems = re.findall(r"^  \} (\w+)\{\*?this\};", drv, re.M)
    comm = (ROOT / COMM).read_text()
    spi_part = comm.split("class UartCommInterface", 1)[0]
    hooks = []
    for name in re.findall(r"static_cast<(?:const )?Derived \*>\(this\)->(\w+)\(", spi_part):
        if name not in hooks and name not in ("ReadRegister", "WriteRegister"):
            hooks.append(name)
    return {
        "subsystems": subsystems,
        "port_hooks": hooks,
        "sources": [f"{DRIVER}:TMC51x0 members", f"{COMM}:SpiCommInterface"],
    }


def build(d):
    f = Fig(
        "driver-layers",
        "Three layers, one of them yours",
        alt="Layer diagram: the application calls the hardware-agnostic TMC51x0 driver and its subsystems, which "
        "use the SPI or UART communication interface, which calls the platform port you write, which drives the "
        "SPI bus or single-wire UART and the DRV_ENN pin of the TMC5160 or TMC5130.",
        width=1200,
        kicker="Architecture",
        subtitle="The driver and the interface are the same on every MCU. The port is a small class you write once.",
        profile="overview",
        legend=[("brand", "this library"), ("teal", "you write"), ("slate", "hardware")],
    )
    W = f.body_w
    app = f.card(0, 0, W, "Your application", ["motion sequence, homing, fatigue test, CLI ..."], tone="slate")
    top = app.b + 40
    chips_h = f.chips(16, f.region_inner_top(top), W - 32, d["subsystems"], tone="brand")
    reg = f.region(0, top, W, f.region_inner_top(top) - top + chips_h + 18, "Hardware-agnostic core", tone="brand",
                   sub="TMC51x0<Comm>, inc/tmc51x0.hpp")
    comm = f.card(0, reg.b + 40, W, "Communication interface",
                  ["SpiCommInterface / UartCommInterface: 40-bit SPI datagrams, daisy-chain padding,",
                   "UART frames with CRC8 and node addresses, control pins by name with per-board levels"],
                  tone="brand", accent=True)
    port = f.card(0, comm.b + 40, W, "Platform port (your class)", ["  ".join(h + "()" for h in d["port_hooks"])],
                  tone="teal", accent=True, mono=True)
    hw = f.card(0, port.b + 40, W, "TMC5160 / TMC5130",
                ["SPI mode 3 up to 4 MHz (internal clock), or single-wire UART  /  DRV_ENN, DIAG0, DIAG1"],
                tone="slate")
    for a, b in ((app, reg), (reg, comm), (comm, port), (port, hw)):
        f.connect(a, b, "bottom", "top", tone="rail")
    return f
