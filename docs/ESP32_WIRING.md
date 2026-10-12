# CAN bench: wiring and how to run it

The bench replays the Car Hacking Dataset onto a real CAN bus with the original
timing, and records every frame that arrives.

```
PC --USB--> ESP32 A  replay_sender  ==== CAN bus ====  ESP32 B  usb_receiver --USB--> PC
```

- ESP32 A plays the car: every frame of a dataset file, normal and attack,
  sent at its original time (each frame ends when it ended in the car).
- ESP32 B is the logger: it stamps every frame it hears with a microsecond
  time and sends it to the PC.
- The PC compares what arrived with what was sent and writes a results page.

Both boards use the ESP32's built-in CAN controller (TWAI). The CAN modules
are transceivers only (pins TX, RX, VIN, GND, CANH, CANL).

## Wiring

Each ESP32 to its own CAN module:

| CAN module | ESP32 |
|---|---|
| TX | GPIO 5 |
| RX | GPIO 4 |
| VIN | 3V3 for an SN65HVD230, 5 V (VIN) for a TJA1050 / TJA1051 |
| GND | GND |

A 5 V module drives RX at 5 V: put a 1 kohm / 2 kohm divider on the RX wire
(module RX - 1 kohm - GPIO 4 - 2 kohm - GND).

Between the modules: CANH to CANH, CANL to CANL, GND to GND. A 120 ohm
termination at each end of the bus only. Check with a multimeter, power off:
CANH to CANL should read about 60 ohm.

## Software

- Arduino IDE, ESP32 board package (core 3.x), board "ESP32 Dev Module". No
  extra library.
- Upload `esp32/replay_sender` to ESP32 A and `esp32/usb_receiver` to ESP32 B.
- Python with `numpy` and `pyserial` (`pip install numpy pyserial`).
- Dataset files (`DoS_dataset.csv`, `Fuzzy_dataset.csv`, `gear_dataset.csv`,
  `RPM_dataset.csv`) in `data/car_hacking/`.

## Run

Close every Arduino Serial Monitor first (the script needs the COM ports).

Easiest: double-click `run_check.bat` (in `Desktop\esp32 setup\can_replay`).
It finds which board is which, runs a 300 frame link check, replays the
dataset and opens the results page. Options from a terminal:

```
run_check.bat --file gear_dataset.csv --n 100000
```

Or step by step, from `python/`:

```
python replay_dataset.py --sender COM8 --receiver COM7
python replay_dataset.py --sender COM8 --receiver COM7 --file Fuzzy_dataset.csv --n 100000
```

Each run writes `runs/<time>_<file>/`: `report.html` (results page),
`frames.csv` (every frame sent, arrived or not, timing), `received.csv` (every
frame heard), `summary.json`.

## Wiring check without any script

`esp32/link_sender` and `esp32/link_receiver`: the sender puts a counting frame
on the bus every 200 ms, the receiver prints every frame in the Serial Monitor
(115200 baud). If the counter goes up line by line, the wiring is good.

## Keeping the copy in sync

The repo is the master copy. `python python/sync_bench.py` copies the bench
files to `Desktop\esp32 setup\can_replay`, where the runs are done.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| "Access is denied" on a COM port | The Arduino Serial Monitor (or the IDE) still has it open |
| A board never prints READY | Wrong sketch, or wrong port: unplug one board to see which port disappears |
| Link check fails | 60 ohm check, CANH/CANL crossed, missing GND, module not powered |
| Lost frames with `drop=` > 0 on the logger | USB could not keep up; should not happen at 500 kbit/s |
| `late=` > 0 on the sender | The sender could not keep the original timing (bus busy) |
