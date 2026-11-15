# Live demo: checklist

About 12 minutes on stage, 20 minutes of setup before. Everything runs from
`run_demo.bat` in the project folder (`Desktop\CAN_IDS_IPS`). Before every step
it says which switch or jumper to set and waits for Enter.

## What the audience sees

1. **A secure model update on the real board.** The FPGA runs a model from its
   own flash. The STM32 update tool pushes a new, signed and scrambled version:
   refused while the maintenance switch is off, accepted when it is on (the
   flash slot LEDs switch), an older version refused, a forged file refused.
2. **The STM32 changes job** and becomes the second CAN detector.
3. **Attacks on a real CAN bus,** real car traffic: without protection every
   attack frame gets through; with the FPGA (running the model just pushed)
   every one is destroyed on the wire and no normal frame is lost; then the
   STM32 does the same in software.

Websites to show (the script offers to open them): the walkthrough
`https://charbel-j-estephan.com/CAN-IDS-IPS` before part 1 (step 5, Update),
the STM32 vs FPGA race `https://charbel-j-estephan.com/CAN-COMPARE` at the end.

## Setup (before the talk)

| Check | |
|---|---|
| Three ESP32s on the CAN bus: sender, attacker, receiver (`docs/ESP32_WIRING.md`) | ☐ |
| DE10-Lite on the bus: pin 1 CAN RX through the divider, pin 2 CAN TX through the level shifter, GND. **No jumper from pin 1 to pin 29** | ☐ |
| STM32 on the bus with its own TJA1050: RX to PB8, TX to PB9, 5 V, GND (`docs/STM32_WIRING.md`) | ☐ |
| **Five SPI wires** STM32 to DE10-Lite, GND first: pin 12 GND, pin 5 PB13, pin 6 PB15, pin 7 PB14, pin 8 PB12 (`updatable fpgas/docs/SPI_WIRING.md`) | ☐ |
| One 120 Ω terminator at each end of the bus; power off, CANH to CANL reads about 60 Ω | ☐ |
| USB: three ESP32s, the STM32 and the DE10-Lite to the laptop | ☐ |
| DE10-Lite: LEDR4 or LEDR5 on (a model from flash), **SW0 down, SW1 down** | ☐ |
| `Desktop\master.key` present (the demo signs with it); every Arduino Serial Monitor closed | ☐ |
| Run **`run_demo.bat prepare`**: it puts the update tool on the STM32 (BT0 jumper to 3V3 + RST, then back to GND + RST when asked) and shows what the FPGA runs | ☐ |

The FPGA keeps its design and its model through a power cycle, so no Quartus
step is needed on the day.

## Run: `run_demo.bat`

| Step | Do | Say | Expected |
|---|---|---|---|
| 1 | SW1 down | “This is what the FPGA runs, from its own flash.” | model, version N, slot A or B |
| 2 | keep SW1 down | “The update port only opens with a physical switch.” | refused: maintenance mode is off |
| 3 | **SW1 up** | “A new model, signed for this board and encrypted.” | accepted, version N+1, LEDR4 / LEDR5 switch slot |
| 4 | keep SW1 up | “An attacker re-sends an old model with a known weakness.” | refused: older version (rollback) |
| 5 | keep SW1 up | “A model signed with the attacker’s own key.” | refused: stamp does not match; 1 bad stamp counted |
| – | **SW1 down**, unplug the 5 SPI wires (GND last), BT0 to 3V3 + RST; when told, BT0 to GND + RST | “The update tool holds no keys. Now the STM32 becomes a detector.” | IDS firmware flashed in a few seconds |
| 6 | SW0 down | “The attack with nothing protecting the bus.” | DoS: every attack frame arrives |
| 7 | **SW0 up** | “The FPGA, with the model we just pushed, destroys attacks on the wire.” | DoS 1,861 of 1,861 destroyed, 0 normal lost, attacker bus off |
| 8 | keep SW0 up | “Spoofed engine speed: a real ID with fake data. Only the trees can tell.” | RPM 1,085 of 1,085 destroyed, 0 normal lost |
| 9 | **SW0 down** | “The same model in software on a 168 MHz microcontroller.” | DoS 1,861 of 1,861 destroyed by the STM32 |

On each results page point at two numbers: attack frames lost (= destroyed)
and normal frames lost (must be 0).

To run the demo again: `run_demo.bat prepare` (the STM32 back to the update
tool), then `run_demo.bat`. Each run pushes a new version, so it never runs
out. Only part 3: `run_demo.bat attacks` (the STM32 must run the IDS firmware).

## If something goes wrong

| Sign | Fix |
|---|---|
| “The FPGA does not answer (all FF)” | an SPI wire: reseat all five, pin 8 (CS) and GND first |
| “not running the update tool” | run `run_demo.bat prepare` |
| “did not appear in 60 s” after a jumper step | BT0 jumper position, then press RST |
| Step 3 refused with “port locked” | three bad stamps since power on: unplug the DE10-Lite USB, plug it back |
| “Could not find exactly one sender and one receiver” | a USB cable, or a Serial Monitor open; replug the ESP32s |
| Link check FAILED | a bus wire or terminator: CANH/CANL, the 60 Ω |
| Step 7 or 8: attack frames arrive | SW0 is down, or the FPGA’s pin 1 / pin 2 wiring |
| Results page did not open | `runs\<date>_<file>\report.html` in the project folder |

Spare results for slides, all from 2026-11-15: `Desktop\esp32 setup\can_replay\runs\20261115_*`
(listed in `docs/PAPER_RESULTS.md`).
