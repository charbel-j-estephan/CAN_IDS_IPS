# Handover: CAN bench status

Branch `claude/esp32-can-bus-wiring-372c60` (worktree
`C:\Users\Charbel\Desktop\CAN_IDS_IPS\.claude\worktrees\esp32-can-bus-wiring-372c60`).
Committed locally, not pushed, not merged into `main`.
The repo is the master copy. Runs are done from `Desktop\esp32 setup\can_replay`;
`python python/sync_bench.py` copies the bench there. How to wire and run it:
`docs/ESP32_WIRING.md`.

## The bench

```
PC --USB--> ESP32 A  replay_sender  (normal frames) --+
PC --USB--> ESP32 C  attack_sender  (attack frames) --+== CAN bus ==  ESP32 B usb_receiver --USB--> PC
```

One click: `can_replay\run_check.bat` finds the boards, runs a 300 frame link
check, replays the dataset with its original timing and opens the results page.
Without C plugged in it runs the 2-board mode (A sends everything).

## Status

| Item | Status |
|---|---|
| 2-board replay | Tested: DoS 20k, Fuzzy 20k, gear 100k, 100% delivered, median 6 to 7 us, 95% within 13 to 22 us |
| 3-board replay (separate attacker) | Tested twice: 20,000 of 20,000 delivered, no errors on any board |
| Sync fix (normal sender takes time zero when the marker finished sending) | Tested (`20261012_223152`): attacker synced 9.7 us from A, was 77 us |
| Drift tracking (attacker follows A's clock during the run) | Committed, **not yet tested on hardware** |

After the sync fix the attacker started in step but its clock ran 6.4 ppm off
A's, so attack frames fell 6 us per second further behind (about 80 us after
14 s) and dragged the normal frames queued behind them: normal median 26 us,
attack frames 3 us. The fix: the PC sends C a reference for every normal frame
(when it should end), C compares with when it heard it and shifts its schedule
(earliest frame per 250 ms window, drift measured from the start). Offline, on
the recorded run, this followed the true drift within about 3 us.

Still unexplained, smaller: A's normal frames read 20 to 30 us late at the
start and while the attack pauses, against about +3 us during the attack
(also faintly in the 2-board runs).

## To do next

1. Re-upload `attack_sender` to the attacker (COM9), run `run_check.bat`.
   Expect the attacker line "followed the normal sender's clock" with about
   6 ppm, and normal frames back near the 2-board timing (median 6 to 7 us).
   Then the bench is final for now.
2. STM32 alone IDS (report Fig. 3), per the approved plan: M1 bit-level CAN
   reception in software on PB8 (TX on PB9 held high), frames to the PC over
   USB in the logger's format; M2 the emlearn random forest per frame with
   decision time; M3 later, error flag for prevention.
3. Later bench additions, only when needed: start the replay at frame N (to
   replay only frames the model never trained on), normal-only replay for false
   alarms, attacker behaviour after bus off for the IPS tests.

## Hardware notes

- Boards: A = COM8 (normal), B = COM7 (logger, coloured breadboard), C = COM9
  (attacker); the numbers can change when replugging, `run_check` finds them.
- CAN modules: TX to GPIO 5, RX to GPIO 4. CANH and CANL on two breadboard
  rails, grounds joined, 120 ohm only on the two end modules (about 60 ohm
  CANH to CANL with the power off).
- A badly wired module kills the whole bus: the sender then shows `tec=128`
  (nobody acknowledged) and the logger hears nothing. Pull modules off the
  rails one at a time to find it.
- Close every Arduino Serial Monitor before running (COM port "Access is denied").
- STM32 (black F407VET6): upload over USB DFU: BOOT0 jumper (BT0 to 3V3), RST,
  Upload in PlatformIO, remove the jumper, RST. Zadig driver must be libusbK.
  ST downloads (CubeIDE) are blocked in your country; use PlatformIO in VS Code.

## Known limits

- The dataset's timestamps are noisy (some gaps are physically impossible), so
  timing cannot be judged finer than about 10 us.
- The trained model uses ID, DLC and data only, with a random split; retrain
  with a time split and the report's features before any accuracy figure goes
  into the report.
- Icarus Verilog is in `C:\iverilog\bin`, but the PATH says `C:\iverlog\bin`.
- Uncommitted, paused: Verilog receiver testbench and vectors
  (`hdl/can_rx_tb.v`, `hdl/vectors/`, `python/make_can_vectors.py`).
