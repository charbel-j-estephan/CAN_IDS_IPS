# Two ESP32s on a real CAN bus

Test the IDS on a physical bus without a car. ESP32 A replays the dataset onto
the bus, ESP32 B runs the same random forest on every frame it hears, and the
PC scores B's answers against the true labels.

```
              USB                                          USB
  PC  <──────────────>  ESP32 A  ──SPI──  MCP2515 A  ═══ CAN ═══  MCP2515 B  ──SPI──  ESP32 B  <──────────────>  PC
 (reads dataset,       (sender)           + TJA1050    CANH/CANL   + TJA1050            (receiver +            (collects classes,
  scores result)                                                                         random forest)          scores result)
```

You need two ESP32 DevKit boards (classic ESP32, not S2/S3/C3), two MCP2515 +
TJA1050 CAN modules, two USB cables, jumper wires, and for each module two
resistors (1 kΩ and 2 kΩ) or a 4 channel level shifter.

## Wiring

Both modules are wired to their ESP32 the same way.

| MCP2515 module | ESP32 DevKit | Notes |
|---|---|---|
| VCC | **VIN** (5 V from USB) | The TJA1050 needs 5 V. 3.3 V does not work. |
| GND | GND | |
| CS | GPIO 5 | |
| SCK | GPIO 18 | |
| SI (MOSI) | GPIO 23 | |
| SO (MISO) | GPIO 19 | **Through a divider**, see below. |
| INT | GPIO 4 | **Through a divider.** Receiver only, leave open on the sender. |

**Level shifting.** With VCC at 5 V the module drives SO and INT at 5 V, and
ESP32 pins are not 5 V tolerant. For each of those two wires:

```
 module SO ──[ 1 kΩ ]──┬── ESP32 GPIO 19
                       │
                     [ 2 kΩ ]
                       │
                      GND
```

(Same for INT to GPIO 4.) The ESP32's 3.3 V on SCK, SI and CS is below the
MCP2515's nominal 3.5 V input threshold at 5 V supply, but it works on nearly
all of these boards. If you see random init failures or garbled frames, use a
4 channel bidirectional level shifter on all five signal wires instead.

**The bus between the two modules** (three wires):

| Module A | Module B |
|---|---|
| CANH | CANH |
| CANL | CANL |
| GND | GND |

CANH to CANH, not crossed. Keep the 120 Ω termination jumper (J1 or similar) on
**both** modules, since they are the two ends of the bus. Check with a
multimeter, power off: CANH to CANL should read about **60 Ω**. 120 Ω means one
terminator is missing, an open reading means a wire is loose.

Plug both ESP32s into the PC with separate USB cables. Each shows up as its own
COM port. Note which is A and which is B.

## Software

1. Arduino IDE with the ESP32 board package, board "ESP32 Dev Module".
2. Library Manager: install **mcp_can** by coryjfowler.
3. Look at the crystal on your CAN modules. If it says 16.000 instead of 8.000,
   change `CAN_CLOCK` to `MCP_16MHZ` in **both** sketches.
4. `pip install pyserial` next to the project requirements.

## Run

```bash
# Train on the real dataset and export the model (see BUILD_GUIDE.md)
python python/train.py --data data/car_hacking --trees 100 --max-depth 10
python python/export_c.py

# Put the model next to the receiver sketch
python python/esp32_eval.py --copy-model
```

Flash `esp32/sender/sender.ino` to ESP32 A and `esp32/receiver/receiver.ino` to
ESP32 B. If B's sketch is too big for the default partition, choose
"Huge APP" under Tools, Partition Scheme, or train with fewer trees. Close the
Arduino serial monitor, then:

```bash
python python/esp32_eval.py --sender COM5 --receiver COM6 --n 20000
```

Use `--n 0` for the whole held out test set. At 500 kbit/s a bit lasts 2 us, so a full
8 byte frame (about 110 bits) takes roughly 220 us. The bus carries at most about
4,500 such frames per second, so 20,000 frames takes around 5 seconds.

The script prints accuracy against the dataset's labels, per class precision
and recall, the confusion matrix, and the detection and false positive rate
treating any attack class as an alarm. It also prints how often ESP32 B agreed
with the Python model: anything below 100 % means the C port or a bus error,
not the model, is responsible.

Without hardware, `python python/esp32_eval.py --selftest` checks the parsing
and scoring code.

## How it works

- The frames come from `X_test` saved by `train.py`, the 20 % the forest never
  trained on. Accuracy on the training frames would be meaningless.
- PC to A uses 11 byte packets and an ack every 64 frames, so the ESP32 serial
  buffer cannot overflow.
- B sends the PC 4 bytes per frame: sync byte, class, CAN ID. The ID lets the
  PC line each answer up with the frame it sent, even when a frame is lost.
  Lost frames are counted and left out of the scores.
- The label never travels on the bus. B classifies from the CAN ID, DLC and
  data bytes alone, and only the PC knows the true label.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| `CAN init failed` forever | Wrong crystal setting, SPI wires swapped, or module not on 5 V |
| Sender stalls, script says "Sender stalled" | Nothing ACKs on the bus: B not running, CANH/CANL swapped, missing GND, wrong bit rate |
| Script never sees READY | Wrong COM port, serial monitor still open, or the sketch is not flashed |
| Frames lost, or `ovf=1` | Receiver too slow for back to back frames. Raise `GAP_US` in the sender (try 100), or use fewer trees / less depth, or check the SPI wiring |
| Bus errors, random resets | Check VIN under load. Some DevKits give about 4.5 V after the USB diode, below the TJA1050's 4.75 V minimum. Power the modules from a true 5 V supply (common GND) |
| Agreement with Python below 100 % | Receiver sketch built with an older `can_ids_model.h`: rerun `--copy-model` and reflash |

## Variant: plain transceiver modules (TX, RX, VIN, GND, CANH, CANL)

If your CAN module has no SPI pins and no MCP2515, the ESP32's built in CAN
controller (TWAI) does the work and the module is only the transceiver. Use
`esp32/sender_twai` and `esp32/receiver_twai` instead of the MCP2515 sketches.
Everything else (PC script, bus wiring, scoring) is the same.

| Module pin | ESP32 |
|---|---|
| TX | GPIO 5 |
| RX | GPIO 4 |
| VIN | 3V3 for an SN65HVD230, 5 V for a TJA1050 / TJA1051 |
| GND | GND |
| CANH, CANL | the other module's CANH, CANL |

- No library to install, no SPI wires, no `CAN_CLOCK` to set.
- A 5 V transceiver drives RX at 5 V. Put the 1 kΩ / 2 kΩ divider from the
  MCP2515 section on that one wire.
- If nothing is received, try swapping the TX and RX wires: some modules label
  them from the module's side.
- Terminate both ends of the bus with 120 Ω. Many of these modules have a
  jumper or a 120 Ω resistor on board, check with the multimeter test above.
- The receiver reports `ovf=<n>`, the number of frames the controller dropped.


## STM32 gateway between the two buses (IPS)

The STM32F407 sits in the middle and decides which frames get through:

```
PC -> ESP32 A -- bus 1 -- [ STM32 CAN1 -> forest -> CAN2 ] -- bus 2 -- ESP32 B -> PC
```

Four transceiver modules (TX, RX, VIN, GND, CANH, CANL), one per CAN port.

| Module | Connects to | Bus |
|---|---|---|
| ESP32 A module | GPIO 5 (TX), GPIO 4 (RX) | bus 1 |
| STM32 module 1 | **PB9** (TX), **PB8** (RX) = CAN1 | bus 1 |
| STM32 module 2 | **PB13** (TX), **PB12** (RX) = CAN2 | bus 2 |
| ESP32 B module | GPIO 5 (TX), GPIO 4 (RX) | bus 2 |

Each bus is its own pair of wires: bus 1 is ESP32 A's CANH/CANL joined to STM32
module 1's CANH/CANL, bus 2 is STM32 module 2 joined to ESP32 B's module. Join
the grounds of everything. Each bus has exactly two nodes, so keep the 120 ohm
termination on every module. The modules are powered separately for 3.3 V or
5 V as in the table above (a 5 V module needs a divider on its RX wire into
the ESP32; the STM32's PB8 and PB12 tolerate 5 V).

PA11 and PA12, the STM32's default CAN pins, are the USB lines on this board,
so the gateway uses PB8/PB9 and PB12/PB13.

Steps:

1. Train on the real dataset and export: `python python/train.py --data
   data/car_hacking ...`, `python python/export_c.py`, then
   `python python/esp32_eval.py --copy-model`.
2. Flash `esp32/sender_twai` to ESP32 A and `esp32/passthrough_twai` to ESP32 B.
3. Build and load `mcu/stm32_gateway` with PlatformIO (BOOT0 jumper on, plug in,
   RST, Upload, then remove the jumper and press RST).
4. Run, with the STM32 powered and running:

```bash
python python/esp32_eval.py --gateway --sender COM5 --receiver COM6 --n 20000
```

A frame that arrives on ESP32 B was let through. A frame that never arrives
was blocked. The script reports the share of attack frames blocked, the share
of normal frames wrongly blocked, and agreement with the Python model.

LEDs on the STM32 board: D2 flickers while frames are forwarded, D3 while
frames are blocked. Both blinking together means setup failed (the bus clock
did not match the bit timing).

Set `BLOCK_ATTACKS` to 0 in `main.cpp` to turn the gateway into a plain
forwarder (detection only, nothing blocked).

## Realistic replay with every frame recorded (WiFi receiver)

Replays a dataset file with its original timing and keeps every frame the
receiver hears. ESP32 B needs no USB data link: it streams over WiFi.

```
PC --USB--> ESP32 A (replay_sender) --CAN bus--> ESP32 B (wifi_receiver) --WiFi--> PC
```

Sketches (both compile with ESP32 core 3.2.0): `esp32/replay_sender` for A,
`esp32/wifi_receiver` for B. The CAN wiring is the same as the TWAI section.

1. Put the PC and ESP32 B on the same hotspot.
2. In `esp32/wifi_receiver`, copy `secrets.example.h` to `secrets.h` and fill in
   the hotspot name, password and the PC's IPv4 address on that hotspot
   (`ipconfig`). `secrets.h` is ignored by git.
3. Flash B first (power only is enough afterwards), then A. Close any Serial Monitor on A.
4. From `python/`:

```bash
python replay_dataset.py --sender COM5                       # 20,000 frames, original timing
python replay_dataset.py --sender COM5 --file Fuzzy_dataset.csv --n 100000
python replay_dataset.py --sender COM5 --fast                # ignore timing, flat out
```

Start the script first: it waits for ESP32 B to connect, then starts the
replay. Allow Python through the Windows firewall on private networks when asked.

Each run writes `runs/<time>_<file>/`:

| File | Contents |
|---|---|
| `frames.csv` | EVERY frame sent: capture time, ID, data, class, arrived or lost, receive time, timing error |
| `received.csv` | every frame ESP32 B heard, in order, with its raw microsecond timestamp |
| `summary.json` | delivery rate, per-class counts, timing error, sender and receiver reports |
| `report.html` | results page: charts, per-class table, searchable table of every frame |

How the timing is measured: ESP32 A sends each frame at its original capture
time (microsecond clock). ESP32 B stamps each frame the moment it reads it from
the CAN controller. The page compares the gaps between frames on the bus with
the gaps in the original capture. The sender reports how many frames left more
than 100 microseconds late. The timestamp is only taken once the frame has been
read, so expect a few tens of microseconds of measurement noise.
