# FPGA test day: 2 hours, step by step

Everything is ready on the PC:
- **Main design: 8 data bytes.** It is compiled (`quartus_board/can_ids_top.sof`) and passes the 8-byte simulation, including the IPS destroying 100 of 100 attack frames. Timing is met: 89 MHz worst case against the 50 MHz needed.
- **Pin test** (`quartus_pintest/`): a tiny design that only checks the perfboard wiring.
- **The GUI** (`run_GUI.bat`) has buttons for both: "Program pin test (wiring)" and "Program the board".

Do the steps in order. If a step fails, stop and fix it before going on (see "If something is wrong" at the end).

---

## Tonight (5 min, if the DE10-Lite is with you)

Plug the DE10-Lite into USB and open the GUI. The status should say **FPGA: USB-Blaster [USB-0]**.

If it says "no USB-Blaster":
1. Open Device Manager and find "USB-Blaster" with a yellow mark.
2. Choose Update driver → Browse → `C:\altera_lite\25.1std\quartus\drivers\usb-blaster`.

Fixing the driver tomorrow would cost 20 minutes.

**Also tonight: upload `esp32/attack_sender` to the attacker ESP32** (version 2). Only version 2 prints `busoffs=` in its report, which is how you'll know the FPGA pushed the attacker off the bus. When it starts, it must print `READY ATTACKER 2`.

---

## 0:00 to 0:20: power off, multimeter only

**Nothing plugged in.** The DE10-Lite pins are not 5 V tolerant, and a short here is the most likely way to lose the morning.

Check the header pin numbers against the GPIO header figure in the DE10-Lite manual. Transceiver pins are for the SN65HVD230 chip itself: 1 D, 2 GND, 3 VCC, 4 R, 6 CANL, 7 CANH, 8 Rs. On a breakout module, follow its labels instead.

| # | Check | Expected |
|---|---|---|
| 1 | Header pin 1 (GPIO[0]) to transceiver **R** (pin 4) | beep |
| 2 | Header pin 2 (GPIO[1]) to transceiver **D** (pin 1) | beep |
| 3 | Transceiver **VCC** to header **3.3 V** (pin 29), **not 5 V** (pin 11) | beep to 3.3 V, none to 5 V |
| 4 | Transceiver GND to header GND (pin 12 or 30) | beep |
| 5 | **Rs** (pin 8) to GND (a module usually does this already) | beep. If Rs floats, the FPGA can receive but never destroy a frame |
| 6 | Each header wire to its neighbours, and 3.3 V / 5 V to GND | **no** beep |
| 7 | CANH to CANL on the whole bus, everything connected and off | **about 60 Ω**. About 40 Ω means one 120 Ω terminator too many (a module often has one). Open means a broken bus wire |
| 8 | Perfboard GND to the ESP32 bench GND | beep (one common ground) |

---

## 0:20 to 0:35: pin test, the FPGA alone on the perfboard

1. Plug the DE10-Lite into USB, with the perfboard connected but the bus idle (ESP32s not sending).
2. Measure transceiver VCC to GND with the meter: **3.3 V**.
3. GUI: **Program pin test (wiring)**.

| LED | Expected now |
|---|---|
| LEDR9 | blinks once a second (the FPGA is programmed) |
| LEDR0 | **ON**: RX reads recessive, the idle bus |
| LEDR1, LEDR2 | off (no traffic) |

4. **TX echo test:** flick **SW1 up for one second, then down.** The transceiver echoes what it drives back to RX.
   - **LEDR3 on = echo OK**: TX, RX, power and Rs are all good.
   - **LEDR4 on = echo FAIL**: see the table at the end.
   - While SW1 is up, CANH − CANL should measure about **2 V** (0 V when SW1 is down).
   - **Never leave SW1 up**: it holds the whole bus dominant.

---

## 0:35 to 0:45: pin test with bus traffic

1. **Unplug the STM32's USB** (power it off). If its prevention is on, it would destroy frames too and mix up the results.
2. Run `can_replay\run_check_esp32.bat` (or GUI → Bench tests → ESP32 bench), with the FPGA still running the pin test.

| Expected | |
|---|---|
| LEDR1 | on during the run (RX sees traffic) |
| LEDR2 | on during the run (**500 kbit/s seen**: the shortest pulse is 2 µs, so the FPGA reads bits at the right speed) |
| The script | the usual 100% delivered. The FPGA only listens |

---

## 0:45 to 1:00: the real design, prevention OFF

1. **SW0 down** (prevention off; LEDR9 off). **SW1 down.**
2. GUI: **Program the board**. The log should say `FEAT_BYTES=8` with no "WARNING ... changed" line. Then press **KEY0** once (reset).
3. Run `run_check_esp32.bat` again.

| Expected | |
|---|---|
| LEDR0 | flickers (good frames decoded) |
| LEDR1 | flickers when attack frames are on the bus |
| LEDR2 | off (prevention is off) |
| LEDR3 | **off**: no CRC, stuff or form errors. If it flickers, the FPGA misreads bits |
| The script | 100% delivered, the same as without the FPGA |

---

## 1:00 to 1:40: prevention ON, all four attacks

**SW0 up** (LEDR9 on). Run each file:

```
run_check_esp32.bat --file DoS_dataset.csv --n 5000
run_check_esp32.bat --file Fuzzy_dataset.csv --n 5000
run_check_esp32.bat --file gear_dataset.csv --n 5000
run_check_esp32.bat --file RPM_dataset.csv --n 5000
```

The script now prints a **"Per class"** table on every run.

| Look at | Good result |
|---|---|
| Normal: lost | **0** (or very few). A lost normal frame was wrongly destroyed |
| Attack class: lost | nearly all |
| `Attacker says: ... busoffs=` | **1 or more**. The FPGA destroyed enough frames to push the attacker off the bus |
| LEDR2 | flashed at the start of the attack |

Why most attack frames are "lost": the FPGA destroys about 32 of them, the attacker then goes **bus off**, and the rest are never sent. That is the IPS working as designed. `busoffs=1` proves it was the FPGA and not a wiring fault.

Save each run folder name (printed at the end) for the report.

---

## 1:40 to 2:00: spare time

- Take photos and a short video of the LEDs for the report.
- 6 bytes instead of 8: GUI → Data bytes 6 → **Compile board design** (about 1 minute) → **Program the board** → repeat one or two runs.
- At the end: SW0 down.

---

## If something is wrong

| What you see | Likely cause |
|---|---|
| LEDR9 does not blink | Not programmed: check the GUI log, the USB cable, the driver |
| LEDR0 off with an idle bus | Transceiver has no power, R not connected, or bus held dominant (unplug the other boards to see) |
| LEDR4 (echo FAIL) | D not connected, **Rs not tied to GND**, or transceiver not powered |
| LEDR1 never lights with traffic | **RX and TX swapped** (header pins 1 and 2), CANH/CANL swapped, or the perfboard not on the bus |
| LEDR1 on but LEDR2 off | Traffic seen but pulses are the wrong length: noise or a bad ground |
| Delivery drops below 100% with SW0 **down** | The FPGA disturbs the bus: TX/RX swapped, or a short on header pin 2 |
| LEDR3 flickers in the real design | Bit errors: noisy wiring or ground, a loose bus wire |
| Normal frames lost with SW0 up | False alarms: send me the run folder |
| `busoffs=0` and attack frames arrive | No error flags reach the bus: re-run the echo test (Rs, D wire) |

Send me the script output or the run folder name for anything unclear.
