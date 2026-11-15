# Live demo: checklist

About 10 minutes on stage, 15 minutes of setup before. Rehearsed on the real
bench on 2026-11-15.

## What the audience sees

A real CAN bus (the network inside a car) carrying real traffic recorded from a
car (Car Hacking Dataset, HCRL), with a separate attacker board injecting
attacks. Two detectors sit on the bus, each running the same trained random
forest:

- **the FPGA** (DE10-Lite), the model in hardware, updated earlier over a
  signed and encrypted channel and kept in its own flash;
- **the STM32** (black F407VE), the same model in software.

Either one can destroy an attack frame on the wire, while it is still being
sent, so the car's other computers never accept it. The attacker's own CAN
controller counts each destroyed frame as its error and soon switches itself
off the bus.

## Setup (before the talk)

| Check | |
|---|---|
| Three ESP32s on the bus: sender, attacker, receiver (`docs/ESP32_WIRING.md`) | ☐ |
| DE10-Lite on the bus: pin 1 CAN RX through the divider, pin 2 CAN TX through the level shifter, GND (`docs/HANDOVER.md`, 11-03 wiring). **No jumper from pin 1 to pin 29** | ☐ |
| STM32 on the bus with its own TJA1050: RX to PB8, TX to PB9, 5 V, GND (`docs/STM32_WIRING.md`). BOOT0 jumper (BT0) on GND | ☐ |
| No SPI wires between the STM32 and the DE10-Lite (the model is already in the FPGA's flash) | ☐ |
| One 120 Ω terminator at each end. Power off, CANH to CANL reads about 60 Ω | ☐ |
| USB: three ESP32s, the STM32 and the DE10-Lite to the laptop | ☐ |
| DE10-Lite: **LEDR5 on** (model from flash slot B) and LEDR9 blinking off/on with SW0. SW1 **down** | ☐ |
| Every Arduino Serial Monitor closed | ☐ |

The FPGA keeps its design and its model through a power cycle (both are in its
flash), so no Quartus step is needed on the day. If LEDR4 and LEDR5 are both
off, the board is still starting: wait 2 s or press KEY0.

## Run

Double-click **`run_demo.bat`** in `Desktop\esp32 setup\can_replay`. It first
lists the boards it found, then runs four steps. Before each step it says which
switch to set and waits for a key. Each step replays 5,000 frames (about 3 s)
and opens the results page in the browser.

| Step | Switch | What to say | Expected on the results page |
|---|---|---|---|
| 1. No protection | SW0 **down** | "This is the attack with nothing protecting the bus." | DoS: all 1,861 attack frames arrive |
| 2. FPGA protects | SW0 **up** | "Now the FPGA watches every frame and destroys the attacks on the wire." | DoS 1,861 of 1,861 destroyed, **0 normal frames lost**, attacker `busoff` |
| 3. Harder attack | SW0 **up** | "Spoofed engine speed: a real car ID with fake contents. Only the model can tell." | RPM 1,085 of 1,085 destroyed, 0 normal lost, attacker `busoff` |
| 4. STM32 protects | SW0 **down** | "The same model in software on a 168 MHz microcontroller." | DoS 1,861 of 1,861 destroyed, 0 normal lost, attacker `busoff` |

Talking points while a step runs:
- The decision is made **inside the frame**: the FPGA's model needs 15 clocks
  (300 ns) once the bytes it reads (ID, length, data bytes 0-4) are in, while
  the rest of an 8-byte frame still takes about 100 µs on the wire, so
  there is time to destroy it before it ends.
- 0 normal frames lost: the model does not touch the car's own traffic.
- The FPGA's model came in **stamped and scrambled** (AES-CMAC, AES-CTR): a
  model signed with another key, an older version, or one for another board is
  refused, and three bad stamps lock the update port until a power cycle.

## If something goes wrong

| Sign | Fix |
|---|---|
| "Could not find exactly one sender and one receiver" | a USB cable, or a Serial Monitor still open. Unplug and replug the ESP32s, run again |
| "STM32 USB port ..." | the STM32's USB cable, or BT0 still on 3V3 (move to GND, press RST) |
| Link check FAILED | a bus wire or terminator. Check CANH/CANL and the 60 Ω |
| Step 2 or 3: attack frames arrive | SW0 is down, or the FPGA is not on the bus (pin 1 / pin 2 wiring, the jumper to pin 29 removed?) |
| Step 2: normal frames lost too | FPGA RX wiring (pin 1 divider) |
| Results page did not open | it is in `runs\<date>_<file>\report.html` |

Spare runs for slides, all from 2026-11-15, are in
`Desktop\esp32 setup\can_replay\runs\20261115_*` (listed in `docs/PAPER_RESULTS.md`).
