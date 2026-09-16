# CableCheck

**Is this cable data, or is it power?** CableCheck answers that on a tiny
built-in OLED in about two seconds.

![cablecheck](https://snail3d.com/lab/)

You know the moment: you plug a board into a cable, nothing responds, and you
can't tell if it's the cable, the port, or the code. Half of those cables in
your drawer are *charge-only* — power wires, no data wires — and nothing on
the box says so.

CableCheck is firmware for a small ESP32-C3 board with an on-board OLED. Plug
any cable into it and the screen tells you the truth:

```
┌──────────────────────┐
│        DATA          │   ← a real host is talking on the data lines
│     host linked      │
│             up 0:12  │
└──────────────────────┘

┌──────────────────────┐
│       CHARGE         │   ← power only: dead cable, dead port, or wall cube
│      power only      │
│             up 0:03  │
└──────────────────────┘
```

## How it works

The ESP32-C3's USB port is built into the chip itself and enumerates in
hardware — no firmware work needed. That gives a perfect witness: **any real
USB host sends "start of frame" packets to its devices every 1 ms, forever.**
Charge-only cables physically lack the data wires, so that heartbeat can
never arrive. A power bank can never fake it.

The firmware (via Espressif's `usb_serial_jtag_is_connected()` driver)
watches for the heartbeat and latches the verdict, so a laptop suspending its
port later doesn't flip the answer back.

No soldering. No jumpers. One cable, flash once, done.

## What you need

| Part | Cost | Notes |
|---|---|---|
| **HW-675 board** — "ESP32-C3 0.42 OLED Mini" | ~$6–8 | AliExpress / Amazon. Native USB-C, SSD1306 OLED on I2C GPIO5/GPIO6, blue LED on GPIO8, BOOT button on GPIO9 |
| A USB-C **data** cable (to flash it) | — | ironically, this tool tells you if yours work |
| Arduino CLI or Arduino IDE | free | ESP32 core ≥3.x |

## Flash it

```bash
# arduino-cli
arduino-cli core install esp32:esp32
arduino-cli lib install u8g2

# board flags: ESP32-C3 Dev Module, USB CDC on boot = DISABLED (default)
arduino-cli compile --fqbn esp32:esp32:esp32c3 firmware
arduino-cli upload  --fqbn esp32:esp32:esp32c3 --port /dev/cu.usbmodem2101 firmware
```

Arduino IDE works too: pick *ESP32C3 Dev Module*, leave *USB CDC On Boot* at
*Disabled*, add the U8g2 library, open `firmware/cablecheck.ino`, upload.
If the first upload can't reset the board automatically, hold BOOT while
tapping RESET, release BOOT, then upload again.

## Reading the screen

| Line | Meaning |
|---|---|
| `DATA` / `CHARGE` | verdict: host heartbeat present on the data lines |
| `host linked` / `power only` | plain-words version |
| `up m:ss` | uptime (doubles as OLED burn-in relief) |

**Hidden diagnostic:** hold the BOOT button and the screen shows the raw USB
event register and the driver's live verdict — handy when a case behaves
strangely.

## Tuning for your panel

HW-675 boards ship with two panel batches. The visible 72×40 window sits at a
fixed offset inside the SSD1306's 128×64 memory; the default in the sketch is
`(27, 24)` with a calibration border drawn around it. If the border is
cropped on one side, nudge `OX`/`OY` in `cablecheck.ino` and re-flash — a few
pixels at a time.

## FAQ

**A laptop port works?** Yes — laptops are full USB hosts and send the same
heartbeat as desktops. The one real exception is an "always-on charge port"
that has data switched off; those correctly read `CHARGE`, because that port
is genuinely a charger wearing a computer's coat.

**Good data cable into a wall cube?** Reads `CHARGE`. Correctly: nothing on
the far end can carry data. The light answers "is there a live data
connection," which is the question you actually had.

**Why does it say CHARGE on a cable I know is fine?** If the port suspended
before the board booted, the heartbeat may have ended before the first
listen. Unplug/replug, or hold BOOT and check the diagnostic line — `con 1`
means the driver can see the host right now.

## License

MIT — do what you like, attribution appreciated.

---

Built by Snail3D · [snail3d.com/lab](https://snail3d.com/lab/) ·
support: snailmail3d@gmail.com
