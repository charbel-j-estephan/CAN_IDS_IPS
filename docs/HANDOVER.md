# Handover: CAN bench status

Branch `claude/esp32-can-bus-wiring-372c60` (worktree
`C:\Users\Charbel\Desktop\CAN_IDS_IPS\.claude\worktrees\esp32-can-bus-wiring-372c60`).
Committed locally, not pushed, not merged into `main`.
The repo is the master copy. Runs are done from `Desktop\esp32 setup\can_replay`;
`python python/sync_bench.py` copies the bench there. How to wire and run it:
`docs/ESP32_WIRING.md`.

## The bench

```
PC --USB--> sender    replay_sender  (normal frames) --+
PC --USB--> attacker  attack_sender  (attack frames) --+== CAN bus ==  receiver  usb_receiver --USB--> PC
```

One click: `can_replay\run_check.bat` finds the boards, runs a 300 frame link
check, replays the dataset with its original timing and opens the results page.
Without the attacker plugged in it runs the 2-board mode (the sender sends
everything).

## Status

| Item | Status |
|---|---|
| 2-board replay | Tested: DoS 20k, Fuzzy 20k, gear 100k, 100% delivered, median 6 to 7 us, 95% within 13 to 22 us |
| 3-board replay (separate attacker) | Tested 3 times: 20,000 of 20,000 delivered, no errors on any board |
| Sync fix (sender takes time zero when the marker finished sending) | Tested (`20261012_223152`) |
| Drift tracking (attacker follows the sender's clock during the run) | Tested (`20261012_225632`): drift 6.8 -> 0.2 ppm, 14,225 frames tracked, 0 missed, median 16.6 -> 7.6 us |
| Sync trim 35 us (attacker time zero earlier) | Tested (`20261012_230423`): attacker offset 35 -> 7.5 us; against the 2-board run frame by frame, median deviation 15.4 -> 7.7 us |
| Sync trim raised to 43 us | Tested (`20261012_231036`): attacker offset 1.8 us; against the 2-board run, attack frames +2.6 us, median deviation 7.0 us |
| STM32 M1: software CAN receiver (`mcu/stm32_ids`) | Builds. The decoder's logic was checked on the PC through a line by line Python copy of `can_rx.c` (20,010 frames, CRC and stuff errors, extended, remote, overload frames); the C itself only compiles (no desktop C compiler here: MSVC lacks its headers), and `test/host_test.c` has never run. First bus test 2026-10-13: 300 of 300 with the PC not reading it, 247 of 300 while it was read (IDs 0x400 and up lost: the first interrupt after an idle bus came over 1 us late and misread the start bit). Next build (start bit from its edge, interrupt path in RAM): 0 of 300, worse, RAM suspected. fw 3: back in flash (`-D CAN_RX_IN_RAM` to try RAM), one interrupt per bit, `isr`/`lat`/`nlate`/`fw` in #STAT, and `upload_stm32.bat` restarts it into the bootloader over USB (no BOOT0 jumper). **Upload fw 3 (jumper once) and A/B test with COM11 closed and open** |

The attacker offset is now measured only on frames with a clear bus before
them (100 us idle in the capture); the old number came from fits pulled by the
normal frames the attacker pushed aside and read 9.6 us when the attacker was
really 35 us late (steady 33 to 35 us through the run). Cause: the attacker
stamps the marker when it has received it, the sender when its controller
reports it sent; the two latencies differ.
`SYNC_TRIM_US` in `attack_sender` removes it.

Judge 3-board runs against the 2-board run (`20261012_215932`, same frames,
one clock), not only against the capture: after the trim the script's median
read 11.4 us (was 7.6) although every frame moved closer to the 2-board run.
Normal frames queued behind others differ most (p95 about 57 us), partly
real arbitration: the attacker's ID 0x000 now wins against the sender's next
frame, where one
board sending in file order never contends. Remaining attacker offset about
8 us with trim 35; with 43 it is 1.8 us.

Still unexplained, smaller: the sender's normal frames read 20 to 30 us late at the
start and while the attack pauses, against about +3 us during the attack
(also faintly in the 2-board runs).

## To do next

1. Bench done for now. (Identical frames from one board are now matched in
   order: the 322 us worst case of run 20261012_231036 rescores to 178 us.)
2. STM32 alone IDS (report Fig. 3), wiring in `docs/STM32_WIRING.md`, per the approved plan: M1 bit-level CAN
   reception in software on PB8 (TX on PB9 released: open drain, the TJA1050
   pulls it to 5 V), frames to the PC over
   USB in the receiver's format; M2 the emlearn random forest per frame with
   decision time; M3 later, error flag for prevention.
3. Later bench additions, only when needed: start the replay at frame N (to
   replay only frames the model never trained on), normal-only replay for false
   alarms, attacker behaviour after bus off for the IPS tests.

## Hardware notes

- Boards: sender = COM8 (normal frames), receiver = COM7 (logger, coloured
  breadboard), attacker = COM9; the numbers can change when replugging, `run_check` finds them.
- CAN modules: TX to GPIO 5, RX to GPIO 4. CANH and CANL on two breadboard
  rails, grounds joined, 120 ohm only on the two end modules (about 60 ohm
  CANH to CANL with the power off).
- A badly wired module kills the whole bus: the sender then shows `tec=128`
  (nobody acknowledged) and the receiver hears nothing. Pull modules off the
  rails one at a time to find it.
- Close every Arduino Serial Monitor before running (COM port "Access is denied").
- STM32 (black F407VET6): upload over USB DFU: BOOT0 jumper (BT0 to 3V3), RST,
  Upload in PlatformIO, remove the jumper, RST. Zadig driver for STM32 BOOTLOADER
  is now WinUSB (libusbK gave LIBUSB_ERROR_PIPE). "Unknown USB Device (Device
  Descriptor Request Failed)" means not in DFU mode or a bad cable/port.
- Windows does not pass RTS changes to the STM32's USB port, so the scripts take
  its #STAT line as ready (serial_link.open_board(ready=...)).
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
