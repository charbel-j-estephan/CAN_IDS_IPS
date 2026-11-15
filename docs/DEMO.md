# Live demo: checklist

About 15 minutes on stage, 20 minutes of setup before. Everything runs from
`run_demo.bat` in the project folder (`Desktop\CAN_IDS_IPS`). Before every step
it says which switch or jumper to set and waits for Enter; after every attack it
prints the result in plain words; during the update it prints what to say.

## What the audience sees

1. **The FPGA alone protects a real CAN bus,** real car traffic, the STM32 only
   listening: without protection every attack frame reaches the car; with the
   FPGA every one is destroyed on the wire, no normal frame is lost and the
   attacker ends up off the bus. Then a harder attack (spoofed engine speed).
2. **The STM32 alone** does the same in software, with the same model.
3. **The FPGA's model is updated, step by step:** what the file contains (header,
   scrambled trees, stamp), refused with the maintenance switch off, accepted
   with it on (the slot LEDs switch, and the screen lists the nine things the
   FPGA did), an older version refused, a forged file refused.

Websites to show (the script offers to open them): the STM32 vs FPGA
race `https://charbel-j-estephan.com/CAN-COMPARE` after part 2, the walkthrough
`https://charbel-j-estephan.com/CAN-IDS-IPS` (step 5, Update) at the start of
part 3.

## The web presentation

`docs/presentation/index.html` (double-click, then F for full screen, arrow keys
to move) follows the talk: what CAN is and why it is open, where an IDS/IPS is
needed, the FPGA live, the basic IDS, 5/6/8 bytes, the modular forest and the
twelve update precautions. Its live slides give the command to type:

| Command | What it runs |
|---|---|
| `run_demo.bat run <who> <attack>` | one replay; who = `none`, `fpga`, `basic` or `stm32`, attack = `DoS`, `Fuzzy`, `Gear` or `RPM` |
| `program_fpga.bat basic` | the basic IDS (forest compiled into logic, `quartus_board`) |
| `program_fpga.bat` | back to the updatable design |
| `run_demo.bat update` | the secure update, step by step |

Each replay adds its result to `runs\live_results.js`; "Refresh live results" on
the graph slide shows it next to the recorded bench results.

## Setup (before the talk)

| Check | |
|---|---|
| Three ESP32s on the CAN bus: sender, attacker, receiver (`docs/ESP32_WIRING.md`) | ☐ |
| DE10-Lite with its shield: CAN on pins 1 and 2, the SPI JST on pins 5–8 and 12. **No jumper from pin 1 to pin 29** | ☐ |
| STM32 with its shield: the TJA1050 on PB8 (RX), PB9 (TX), 5 V, GND; **both CANH and CANL** to the bus rails (`docs/STM32_WIRING.md`) | ☐ |
| **SPI cable** between the two shields, both JST plugs pushed fully in (pin 1 GND, 2 SCLK, 3 MOSI, 4 MISO, 5 CS; `updatable fpgas/docs/SPI_WIRING.md`) | ☐ |
| One 120 Ω terminator at each end of the bus; power off, CANH to CANL reads about 60 Ω | ☐ |
| USB: three ESP32s, then the DE10-Lite, then the STM32 (the DE10-Lite before the STM32) | ☐ |
| DE10-Lite runs the project's design: LEDR4 or LEDR5 on. If it runs anything else (the factory demo, another lab), run **`program_fpga.bat`** (about 30 s, USB-Blaster over the board's USB), then unplug and replug the board: LEDR4 and LEDR5 both on (the built-in model). Then **SW0 down, SW1 down** | ☐ |
| `Desktop\master.key` present (the demo signs with it); every Arduino Serial Monitor closed | ☐ |
| Run **`run_demo.bat prepare`**: it puts the IDS firmware on the STM32 if needed (shield off, BT0 jumper to 3V3 + RST; after programming, BT0 off 3V3, shield back on, RST; it waits for Enter after each) | ☐ |

The FPGA keeps its design and its model through a power cycle, so
`program_fpga.bat` is only needed if the board was given another design.
`program_fpga.bat check` checks Quartus, the design file and the cable without
programming. The design file (`updatable fpgas\quartus\output_files\updatable_top.pof`)
holds the board's key: it stays on this PC, not in git.

## Run: `run_demo.bat`

| Step | Do | Expected |
|---|---|---|
| **Part 1** | the script explains the bench and the FPGA (read it out) | |
| 1 | SW0 down | DoS: every attack frame reaches the car (1,861 of 1,861) |
| 2 | **SW0 up** | DoS: 1,861 of 1,861 destroyed, 0 normal lost, attacker bus off |
| 3 | keep SW0 up | RPM: 1,085 of 1,085 destroyed, 0 normal lost |
| **Part 2** | | |
| 4 | **SW0 down** | DoS: 1,861 of 1,861 destroyed by the STM32; then the race website |
| **Part 3** | the STM32 becomes the update tool: shield off, BT0 to 3V3 + RST, Enter; after programming BT0 off 3V3, shield back on, RST, Enter | |
| 5 | SW1 down | the model, version N, slot A or B |
| 6 | - | the update file field by field: header, the trees before and after scrambling, CRC, stamp |
| 7 | keep SW1 down | refused: maintenance mode is off |
| 8 | **SW1 up** | accepted, version N+1, LEDR4 / LEDR5 switch slot; the nine steps the FPGA did |
| 9 | keep SW1 up | refused: older version (rollback) |
| 10 | keep SW1 up | refused: stamp does not match; 1 bad stamp counted |
| 11 | **SW1 down** | port closed |

If a switch is in the wrong place during part 3, the script sees it on the board
and waits until it is right.

After each attack the script prints a short table: attack frames sent, reached
the car, destroyed; normal frames sent, lost; whether the attacker went bus off;
and one sentence to say. The full results page opens too.

To run the demo again: `run_demo.bat prepare` (the STM32 back to the IDS
firmware), then `run_demo.bat`. Each run pushes a new version, so it never runs
out. Parts 1 and 2 only: `run_demo.bat attacks`; part 3 only:
`run_demo.bat update`.

## If something goes wrong

| Sign | Fix |
|---|---|
| “The FPGA does not answer (all FF)” | push both plugs of the SPI cable fully in; unplug both USB cables, then plug the DE10-Lite first, the STM32 second |
| “did not appear in 60 s” after a jumper step | BT0 jumper position, then press RST |
| The STM32 drops off USB mid-demo and Windows shows “STM32 BOOTLOADER” | it restarted with BOOT0 high: BT0 off 3V3, shield on, RST, then run the demo again (nothing was changed on the FPGA) |
| Step 8 refused with “port locked” | three bad stamps since power on: unplug the DE10-Lite USB, plug it back |
| “Could not find exactly one sender and one receiver” | a USB cable, or a Serial Monitor open; replug the ESP32s |
| Link check FAILED | a bus wire or terminator: CANH/CANL, the 60 Ω |
| Step 2 or 3: attack frames arrive | SW0 is down, or the FPGA’s pin 1 / pin 2 wiring |
| Step 4: 0 attack frames destroyed, the STM32 says `rx=0` | the STM32 hears nothing: its module’s CANH or CANL is not on the bus rail (power off, beep each one to the sender’s module) |
| “The replay did not finish” | read the messages above it; most often a USB cable, or a Serial Monitor left open |
| Results page did not open | `runs\<date>_<file>\report.html` in the project folder |

Spare results for slides, all from 2026-11-15: `Desktop\esp32 setup\can_replay\runs\20261115_*`
(listed in `docs/PAPER_RESULTS.md`).
