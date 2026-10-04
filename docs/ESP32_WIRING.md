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

Use `--n 0` for the whole held out test set. The bus carries roughly 5,000 to
9,000 frames per second, so 20,000 frames takes a few seconds.

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
