# CAN bench: wiring and how to run it

The bench replays the Car Hacking Dataset onto a real CAN bus with the original
timing, and records every frame that arrives.

```
PC --USB--> sender  replay_sender  ==== CAN bus ====  receiver  usb_receiver --USB--> PC
```

- The sender plays the car: every frame of a dataset file, normal and attack,
  sent at its original time (each frame ends when it ended in the car).
- The receiver is the logger: it stamps every frame it hears with a microsecond
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
- Upload `esp32/replay_sender` to the sender and `esp32/usb_receiver` to the receiver.
- Python with `numpy` and `pyserial` (`pip install numpy pyserial`).
- Dataset files (`DoS_dataset.csv`, `Fuzzy_dataset.csv`, `gear_dataset.csv`,
  `RPM_dataset.csv`) in `data/car_hacking/`.

## Run

Close every Arduino Serial Monitor first (the script needs the COM ports).

Easiest: double-click `run_check_esp32.bat` (in `Desktop\esp32 setup\can_replay`).
It finds which board is which, runs a 300 frame link check, replays the
dataset and opens the results page. Options from a terminal:

```
run_check_esp32.bat --file gear_dataset.csv --n 100000
```

Or step by step, from `python/`:

```
python replay_dataset.py --sender COM8 --receiver COM7
python replay_dataset.py --sender COM8 --receiver COM7 --file Fuzzy_dataset.csv --n 100000
```

Each run writes `runs/<time>_<file>/`: `report.html` (results page),
`frames.csv` (every frame sent, arrived or not, timing), `received.csv` (every
frame heard), `summary.json`.

## Three boards: a separate attacker

```
PC --USB--> sender    replay_sender  (normal frames) --+
PC --USB--> attacker  attack_sender  (attack frames) --+== CAN bus ==  receiver  usb_receiver --USB--> PC
```

With a third ESP32 the attack frames come from their own node, as in the
block diagram report: they compete with the normal frames in real arbitration
(a DoS frame with ID 0x000 always wins), and later an IPS can push only the
attacker to bus off.

- Upload `esp32/attack_sender` to the attacker. It prints `READY ATTACKER`, which
  is how `run_check` finds it. Wire its CAN module like the others, on the same
  CANH/CANL pair. Keep 120 ohm only on the two modules at the physical ends of
  the bus (multimeter: about 60 ohm CANH to CANL).
- `run_check_esp32.bat` uses the attacker automatically when it is plugged in. By
  hand: `python replay_dataset.py --sender COM8 --receiver COM7 --attacker COM9`.

How the two senders stay in step: the sender first sends a start marker (ID
0x7FF, data "CANSTART"). The attacker starts its clock when it hears it, so both
boards share one time base within a few microseconds, independent of USB.
The two crystals then drift apart by a few ppm (tens of microseconds over a
few seconds), so the PC also sends the attacker a timing reference for every normal
frame. The attacker hears those frames, sees how far its own clock has moved
from the sender's and
shifts its schedule to match for the whole run.
The marker is not counted in the results. The run starts at the first normal
frame of the file.

The results page then shows each board's numbers: frames sent, late frames,
error counters, bus state, lost arbitrations, and how closely the attacker
follows the sender's clock (offset measured on frames with a clear
bus before them, so it is not hidden by frames queued behind others).

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
| Lost frames with `drop=` > 0 on the receiver | USB could not keep up; should not happen at 500 kbit/s |
| `late=` > 0 on the sender | The sender could not keep the original timing (bus busy) |
