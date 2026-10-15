# Gondosóra-DOOM

**DOOM running headless on Gondosóra GPS-emergency watch. Sound through PWM.**

Based on [FrenkelS/Doom64KB](https://github.com/FrenkelS/Doom64KB). The goal: run the game without any video rendering, and add sound (and eventually music) through a PWM output.

Part of an artwork by Bódai András (2026), shown in the group exhibition *MODDING: Here's All The Data!* (Budapest, October 2026). If you scanned a QR code on a wall label, you are in the right place.

---

## Magyarul röviden
 
Ez a projekt a DOOM 1993-as shareware változatát futtatja egy Gondosórán (Nordic nRF52840 MCU). Az eszköznek nincs kijelzője, ezért a játék képernyő nélkül fut: a demó magától pörög, a hang pedig PWM-kimeneten át jut a hangszóróhoz. A kód a FrenkelS Doom64KB projektjén alapul. Lent megtalálod a teljes leírást angolul: a bekötést és a feltöltést.
 
> **Figyelem:** ez a folyamat visszafordíthatatlanul törli az eszköz eredeti szoftverét. Az átalakított óra **nem képes vészjelzést leadni**.

---

## What this is

The Gondosóra is a free emergency-call wearable the Hungarian government issues to people over 65. It has one SOS button, three status LEDs, a speaker, GPS and a GSM modem, and **no display**. It does not even show the time.
 
This project replaces the original firmware with the DOOM engine. The engine runs the attract-mode demo loop (title page, then a recorded demo, repeat) with every draw routine stubbed out. The game simulates normally, and the only output is sound through the original speaker.

- [x] Boots, latches power, runs the engine headless
- [x] Title page and demo loop run on the 35 Hz game clock
- [x] Sound effects through PWM on P0.19

## Read this first
 
- **It is a one-way operation.** The nRF52840 on the board is read-protected (APPROTECT). The only way in is a full chip erase, and the original firmware cannot be backed up. After flashing, the unit **no longer works as an alarm**. It will not call anyone.
- **You may damage the hardware.** You will open the case, solder to a small board, and drive a motor driver as an audio output. Do it at your own risk.
- This project is not affiliated with 4iG, SOS Központ, or the Hungarian government. DOOM is a trademark of id Software / ZeniMax.

## Hardware
 
The board inside is marked **`EV07BG_V3.1`** (speaker flex: `EV07BX_FPC_SPK2_V3.0`). It matches the Eview EV07B family.
 
| Part | What it is |
|---|---|
| MCU | Nordic **nRF52840** (Cortex-M4F, 64 MHz, 256 KB RAM, 1 MB flash), behind a shield |
| Modem | SIMCom **SIM800C** (GSM) |
| Debug access | labelled pads: `SWDIO`, `SWDCLK`, `GND`, `3V3` |
| Speaker | wired to the modem originally, so the nRF cannot drive it as shipped|

### Pins used and pins to leave alone
 
Found by tracing and by driving pins from firmware. There is no public schematic.
 
| Pin | Role |
|---|---|
| P0.02 – P0.05 | **Power latch. Must be driven high at boot** or the board switches itself off |
| P0.19 | **Audio out** (PWM0). Goes to the motor driver, which switches the speaker. Active-high |
| P0.09, P0.10 | NFC antenna pins. Leave alone (they crash the chip) |
| P0.21 | nRESET. Leave alone |
| P0.24, P0.25 | Disturbed the debug link in testing. Leave alone |

## What you need
 
- A Gondosóra unit (board `EV07BG_V3.1`) with its battery fitted
- A Raspberry Pi (tested on a 3B+) to act as the SWD programmer, with [OpenOCD](https://openocd.org/) 0.12
- [SEGGER Embedded Studio](https://www.segger.com/products/development-tools/embedded-studio/) v5.50
- Thin wire, a soldering iron, a multimeter, and a resistor of at least 33 Ω for the speaker

## Install
 
### 1. Open the case and find the debug pads
 
The case is held by three tiny screws. If a screw won't turn, add a drop of isopropyl alcohol and wait or apply heat. Under the shield and the large silver plate are the board and the modem. On the modem side, the pads labelled `SWDCLK`, `SWDIO` and `GND` are the ones you need.

### 2. Wire SWD to the Raspberry Pi

I 3D printed [this pogo pin clamp](https://www.printables.com/model/139112-pogo-pin-clamp-block-for-use-with-flash-adapter-wi) to hold 3 pogo pins in place

| Board pad | Raspberry Pi |
|---|---|
| `SWDIO` | GPIO 24 (pin 18) |
| `SWDCLK` | GPIO 25 (pin 22) |
| `GND` | GND (pin 20) |

### 3. Install OpenOCD on the Pi
 
```bash
sudo apt update
sudo apt install openocd
```
 
All of the commands below use this adapter setup. Save it as `rpi-swd.cfg`, or paste it into each command:
 
```
adapter driver bcm2835gpio
bcm2835gpio speed_coeffs 100000 0
bcm2835gpio peripheral_base 0x3f000000
adapter gpio swclk 25
adapter gpio swdio 24
transport select swd
adapter speed 50
```

### 4. Erase the chip (one-way)
 
```bash
sudo openocd -f rpi-swd.cfg -f target/nordic/nrf52.cfg
```
 
In another terminal:
 
```bash
telnet localhost 4444
> nrf52_recover
```

`nrf52_recover` erases the whole chip, including the original firmware, and clears the read protection. A first connection attempt that says `AP lock engaged` is what confirms the lock was on. After a successful recover, check that you can still connect and read `0x10000100` (the part number, `0x00052840`).

### 5. Build (optional) & Flash

You can build your own .hex file in Segger Embedded Studio if you'd like.

1. Open `64KBDOOM_port.emProject` in SEGGER Embedded Studio 5.50.
2. Select the `Debug` configuration. The target is Cortex-M4 with `FPv4-SP-D16`. `PLATFORM_NRF52840` is defined, and `FLAT_SPAN` is defined for `p_setup.c`.
3. Build. The output is `Output/Debug/Exe/64KBDOOM_port.hex`.

I also included the built .hex file in the repository.

From the folder with the `.hex`, on the Pi:
 
```bash
sudo openocd -f rpi-swd.cfg -f target/nordic/nrf52.cfg \
  -c "init" -c "reset halt" \
  -c "program 64KBDOOM_port.hex verify reset exit"
```
 
You should see `** Programming Finished **` and `** Verified OK **`, and the program starts at once.

### 8. Wire the speaker
 
The original speaker hangs on the modem's audio output, which the nRF cannot reach. The workaround reuses the vibration motor's driver, which the nRF can switch from P0.19:
 
1. Unsolder the vibration motor from the `-VIB+` pads.
2. Cut the flex connecting the speaker to the modem board (it sits in a flex with a two-pad end). Solder thin wires to the speaker's own contacts, which should read around 7 Ω.
3. Connect one speaker wire to the `+VIB` pad. Connect the other to the `-VIB` pad **through a series resistor of at least 33 Ω**.
Without the resistor, the speaker can draw far more current than it or the driver should handle. Keep it connected through the first tests.


License

GNU GPL v2 (see LICENSE)
id Software's game data is not covered by that license, and you are responsible for obtaining and using it under the terms of the shareware release.




