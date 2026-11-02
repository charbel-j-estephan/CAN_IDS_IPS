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
| STM32 M1: software CAN receiver (`mcu/stm32_ids`) | **Works** (fw 3, 2026-10-13): 3-board DoS replay through the STM32, 20,000 of 20,000, 0 new stuff/form/CRC errors, no buffer overflow; longest bit interrupt 417 cycles, latest sample 50 ticks (limit 84), none near the limit. History: fw 1 (two interrupts per bit, start bit sampled) lost IDs 0x400+ while USB was busy (247/300); fw 2 (interrupt in RAM) 0/300. Upload with the BOOT0 jumper; after an upload the USB port fails until RST (BT0 on GND). The "DFU!" restart into the bootloader does not work (board drops off USB) |
| STM32 M2: model on the STM32 (fw 5) | Ran on hardware 2026-10-29: link check failed, 296 of 300 frames thrown away (crc 250, stuff 46): bit samples up to 120 TIM4 ticks late (limit 84, fw 3: 58), longest bit interrupt 465 cycles. Cause (likely): the model runs from flash and its code pushes the bit interrupt's code out of the 1 KB flash cache |
| STM32 M2 fw 6: model in RAM | Failed the same (lat 149, 296 of 300 bad); also with the model switched off, so the model is not the cause. fw 3 re-uploaded the same evening: 20,000 of 20,000, lat 50, no errors: the hardware is fine, fw 5/6 broke the bit timing |
| STM32 fw 7: diagnostic | keep()/features_in() noinline: bit interrupt entry back to 5 saved registers (fw 5/6: 9). Model back in flash (-D MODEL_IN_RAM to move it). Switches over USB: 'Z' zero counters, 'c'/'C' model, 'f'/'F' mid-frame features, 'p'/'P' fw 3 / fw 5 priorities; #STAT adds avg (mean bit interrupt, cycles), slow (> 300 cycles), feat, prio. **Not yet run** |
| STM32 fw 8: prevention (M3) | Error flag on PB9 (TIM4 CH4 output compare, 6 bits at the next bit boundary) when the model says attack, off at power up, 'I'/'i' over USB, marker allow listed, 8 bit safety cut. `run_check_ips.bat` / `replay_dataset.py --ips COMx`: ESP32 receiver records what survived, STM32 counters rej/ilate/allow/abort. PC side tested on fakes; firmware builds (bit interrupt entry still 5 registers). **Not yet run on hardware; fw 7's timing fix not confirmed either** |
| STM32 fw 8/9 on hardware (11-01) | fw 8: 0 of 20,000 decoded even with model, features and fw 5 priorities switched off (mean bit interrupt 255-296 cycles of 336, lat 314). fw 9 (fw 8 with fw 3's per bit work): 19,991 of 20,000, lat 88 against an 84 tick margin. So per bit cost was the cause and fw 3 itself had almost no margin |
| STM32 fw 10: lighter hot path | Sample point a quarter bit after the edge (margin 126 ticks, was 84). M3 costs one byte test per bit (m3_pending, set by the model interrupt only for the frame still on the bus). avg/slow counters gone; feat_end is 0xFFFF when features are off. Builds; bit interrupt 0x16c bytes (fw 3 0x194), 5 saved registers. Ran 11-01: link check failed, 192 good / 251 errors, lat 130, dlate 111, cmax 16,257 |
| STM32 fw 11: model in RAM | fw 10 plus -D MODEL_IN_RAM (default in platformio.ini): the model (10 KB) runs from SRAM, so it no longer evicts the bit interrupt from the flash cache. Ran 11-01 (after a loose STM32 module wire was fixed): 64 of 300, lat 153, dlate 53, every decision 0.08-0.24 us before the frame end: the model gets almost no CPU inside a frame, the bit interrupt takes nearly all of it. Flash cache on, no FPU in the bit path (checked) |
| STM32 fw 12: fewer TIM4 reads | Bit interrupt reads TIM4 twice per bit (SR, CNT; CCR3 on edges) instead of 6-7: the sample time and the interrupt enables are kept in RAM, CC3IF comes from the SR read on entry, can_rx_bit returns CAN_RX_IDLE (no can_rx_idle() call per bit). **Works (11-01)**: link check 300/300; DoS 20,000 replay: 20,000 of 20,000 decoded, 0 bus errors, lat 96, recall Normal 100% / DoS 100%, 0 false alarms, dlate 0, decision median 49.9 us after the 5th data byte (worst 76.0), at least 10.5 us left before the frame end (median 54.1). Run `20261101_183017_DoS_dataset`. M2 done; see the M3 row |
| STM32 M3 on hardware (fw 12, 11-01) | `run_check_ips.bat`, DoS 20,000: normal 14,442 of 14,442 arrived (0 lost); attack 5,553 of 5,558 stopped (99.91%). 21 error flags, attacker bus off within ~16 ms of the first attack (then its remaining frames never reach the bus). 5 attack frames got through, all retransmissions in the first 5 ms; the STM32 did not decode them (crc 5, form 2, lat 281: late samples around the error frames), so no verdict. Normal frames delayed up to 3.5 ms during that burst (11 over 1 ms). abort 0, ilate 0, allow 1 (marker). Run `20261101_183214_DoS_dataset`. Open: the 5 leaks (decoder resync after an error frame) |
| STM32 fw 13: fewer IPS leaks | After an error the decoder takes the bus as idle after 8 recessive bits (the error delimiter; was 11, which left no slack before the resend's start of frame). 11 still at power up. m3_step (with flag_begin/flag_step) and can_rx_before_eof run from RAM: from flash they ran cold around each flag. Ran 11-01: worse, 5,542 of 5,558 stopped (16 leaks), rej 41, lat 819, nlate 1326, ilate 2; normal 0 lost, attacker bus off |
| STM32 fw 14: flag scheduling | m3_step back in flash. flag_begin sets the start compare from a fresh CNT (at least 16 ticks ahead, gives up if more than 2 bits late), the flag steps on CC4IF from the entry SR instead of a CNT read every bit, safety cut by bit count. sample_at re-reads CNT when the next sample is under half a bit away (no more 780 us deaf decoder after a missed compare). idle_need 8 kept. PC: IPS runs keep the STM32's records and print/save (leaks.csv) whether it heard each leaked attack frame and what it decided. Ran 11-01, all four files, 0 wiring trouble: **DoS 5,558 of 5,558 stopped**, 0 normal lost, 22 flags, attacker bus off. **Fuzzy 2,090 of 2,092** (2 not heard by the STM32), 0 normal lost, bus off. **Gear 3,940 of 3,940**, 0 normal lost, bus off. **RPM fails**: 4 of 4,075 stopped, 2 normal 0x316 frames lost, rej 4,565 with abort 4,242 (almost every flag cut by the safety limit), lat 276, nlate 36,657, sender buserr 319, attacker never bus off (tec 117), normal frames up to 18 ms late; leak report says the STM32 heard none of the leaked frames. RPM without prevention (run `20261101_190059_RPM_dataset`): 20,000 of 20,000, 0 bus errors on any node, model 4,075 of 4,075 RPM with 0 false alarms, at least 58 us left on every attack frame. So the bench and the model are fine and the fault is in the flag handling on RPM traffic |
| STM32 fw 15: flag diagnostics | fw 14 plus a #DBG line every second: fstart/fend (start and end matches handled), fpush/fgive (start moved later / given up), catch (samples taken at once), and a snapshot of the first abort (flag state, bits, CNT vs flag_end and CCR4, ticks since flag_begin, CCMR2, CCER, SR, decoder idle). replay_dataset prints it after an IPS run and saves it as ips_debug in summary.json. Ran 11-01 on RPM: 4,019 of 4,075 stopped, 1 normal lost, attacker bus off; fstart=fend=31 (all pushed), 57 aborts, first abort: start compare set 21 ticks ahead of the CNT read and never matched (state 1, CC4IF clear) |
| STM32 fw 16: start compare checked after the write | FLAG_LEAD 40 ticks; after CCR4 is written the counter is read again and, if it passed `at` without a match, the start moves a bit (repeat). fw 15 diagnostics kept. Ran 11-01 on RPM (`20261101_191154_RPM_dataset`): **4,075 of 4,075 stopped**, 32 flags, all started and ended (fstart=fend=32, all pushed a bit later), abort 0, attacker bus off, 0 decode errors. But 3 normal frames (0x130, 0x131, 0x43F) never reached the logger, all within 2 ms of the first attack frame, while the sender reports them sent with tec 0 and no failed sends; not explained yet. catch=15,994 and lat 321: the bit interrupt is often late around the flags |
| STM32 fw 16, all four attacks (11-01) | **DoS 5,558/5,558 stopped, Fuzzy 2,092/2,092, Gear 3,940/3,940, RPM 4,075/4,075**. Normal frames lost: 0, 0, 0 and 3 (RPM, first 2 ms of the attack, open). Attacker bus off in every run (21-32 flags), abort 0, ilate 0, fgive 0, no leaks. Runs `20261101_191502_DoS`, `191606_Fuzzy`, `191650_gear`, `191154_RPM`. **The STM32-alone IDS/IPS (M1-M3) is done**; next is the FPGA |

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
- Random forest (2026-10-13), report settings: 5 trees, depth 9, ID + DLC +
  first 5 bytes, `--split time` (each file cut where 20% of its attack frames
  remain; attacks fill only the first ~80% of every recording, 300 episodes).
  Test: 6.54 M frames, accuracy 0.99916 (hard vote). Recall: DoS, Gear, RPM
  100%, Fuzzy 95.2% (4,745 Fuzzy frames called Normal), Normal 99.99% (669
  Normal called Gear, 51 Fuzzy). 1,385 nodes. Model `models/rf5_d9_p5_time.joblib`,
  C `mcu/generated/can_ids_model_rf5_d9_p5.h` (141 KB). A random split gave
  0.99720, not higher: different test traffic, not comparable.
- Fuzzy fix (2026-10-13): 4,653 of the 4,745 missed Fuzzy frames carried an ID
  the car never sends (the normal traffic uses 38 IDs; 9 comparisons per tree
  cannot single them out of 2048). Unknown ID rule: a Normal verdict on an ID
  not in the training traffic becomes Fuzzy (forest.known_id_rule, a 2048 bit
  table in the C header, `can_ids_id_known()`). Fuzzy recall 95.18% -> 99.91%,
  attacks flagged 98.982% -> 99.980%, false alarms unchanged (720 of 6.07 M,
  0.012%), accuracy 0.99988. Tried and dropped: the known ID bit as a forest
  feature (Fuzzy 99.45% but 3,992 false alarms), the rule before the forest
  (DoS 0x000 is also unknown, so DoS became Fuzzy). 8 bytes instead of 5:
  Fuzzy 99.95%, 55 false alarms, but a later decision (19 bits left, not 43).
  669 of the false alarms are the car's own 0x43F 01 45 60 FF 6C, one off from
  the Gear attack frame (... FF 6B), a value missing from the training traffic.
  Verilog exporter does not have the rule yet.
  Train with `.venv` (Python 3.12: `pip install -r requirements.txt setuptools`;
  emlearn needs setuptools for distutils).
- Icarus Verilog is in `C:\iverilog\bin`, but the PATH says `C:\iverlog\bin`.
- Uncommitted, paused: Verilog receiver testbench and vectors
  (`hdl/can_rx_tb.v`, `hdl/vectors/`, `python/make_can_vectors.py`).

Renamed 11-02 (rows above use the old names): run_check.bat -> run_check_esp32.bat, run_check_stm32.bat -> run_check_stm32_IDS.bat, run_check_ips.bat -> run_check_stm32_IPS.bat, run_sim.bat -> run_sim_FPGA.bat.
