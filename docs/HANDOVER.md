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

0. (11-03) Prove the lost RPM normal frames are the ESP32 receiver's: FPGA with
   SW0 up, STM32 on the bus too (plugged in, any firmware 8 or later), then
   `run_check_esp32.bat --listen --file RPM_dataset.csv --n 5000`. `--listen`
   keeps the STM32's prevention off and records with it next to the ESP32
   receiver; for every normal frame the receiver lost, it prints how many
   identical copies were sent, recorded by the receiver and by the STM32
   ("on the bus: the STM32 heard it, the ESP32 receiver did not", or "not heard
   by either recorder"). Saved as `lost_normal_check` in summary.json. Repeat a
   few times: the loss does not happen every run. (`run_check_stm32_IDS.bat`
   cannot do this: the STM32 replaces the ESP32 receiver there.) Then the FPGA's
   mixed runs (held out frames) need attack_sender version 2.

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
- FPGA (DE10-Lite): header pin 1 = GPIO[0] RX, pin 2 = GPIO[1] TX, 11 = 5 V,
  12 = GND, 29 = 3.3 V. TJA1050 (5 V) with RX through a 5 k / 5 k divider to
  pin 1, TX through TXS0108E A2 -> B2 (VA and OE on 3.3 V, VB on 5 V). The
  design is lost at power off: program it again (GUI) after every replug.
  SW0 = prevention, LEDR9 shows it; KEY0 = reset.
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

| 11-02: mixed runs | `--file mixed` (default of run_check_stm32_IDS/IPS.bat): blocks from all four files in random order, from the held out part of each file (after train.py's time split cut, 59-65% into each file). **All earlier bench runs replayed the first 20,000 frames of a file, which are training frames**, so their 100% recall / 0 false alarms is on training data; the mixed runs are the honest test. attack_sender version 2 (READY ATTACKER 2): block start packets (0xFE) make it recover from bus off between blocks; #TX adds busoffs/recovered. run_sim_FPGA.bat asks for 5/6/8 bytes when double-clicked. Fake bench passes; **not yet run on hardware** |
| 11-02: GUI | `run_GUI.bat` -> `python/bench_gui.py` (tkinter, bench venv). Status from USB lists only (STM32 running 0483:5740 / bootloader 0483:df11 via dfu-util -l / ESP32 count / USB-Blaster via quartus_pgm -l), never opens a port while polling. STM32 upload as a guided jumper sequence (mcu/stm32_ids, then reads fw=). FPGA: byte choice edits FEAT_BYTES and the generated_pN qsf line, run_sim_FPGA.bat N, quartus_sh compile, quartus_pgm with the newest .sof. Tests run can_replay/python/run_check.py; Stop on an IPS run sends 'i' to the STM32. Checked with nothing plugged in (status, job runner, byte edit on a copy). **Upload, compile/program and test buttons not yet used on hardware** |
| 11-03: FPGA wiring | DE10-Lite to a TJA1050 (5 V) through a TXS0108E level shifter. The first perfboard's header did not match the DE10-Lite pins; rebuilt on a breadboard. **TXS0108E B1 -> A1 did not pass RX** (TJA1050 RX toggled at B1, header pin 1 never changed: pin test LEDR1/LEDR2 dark, echo FAIL). Fix: RX bypasses the TXS0108E with a divider, TJA1050 RX -> 5 k -> pin 1 -> 5 k (or 10 k) -> GND (2.5 to 3.3 V). TX still pin 2 -> A2 -> B2 -> TJA1050 TX (echo OK). `quartus_pintest/` (GUI "Program pin test (wiring)") found it; checklist in `docs/FPGA_TEST_DAY.md` |
| 11-03: FPGA, prevention off (SW0 down) | 8-byte design (`FEAT_BYTES=8`, generated_p8; 89 MHz worst case vs 50 MHz, run_sim_FPGA.bat 8 all PASS). ESP32 bench with the FPGA on the bus: **5,000/5,000 delivered, 0 bus errors on every board** (runs `20261103_140146_DoS`, `140227_DoS`): the FPGA listens without disturbing the bus |
| 11-03: FPGA IPS, all four attacks (SW0 up) | **DoS 1,861/1,861 destroyed, Fuzzy 709/709, Gear 922/922, RPM 1,085/1,085.** Normal frames lost: 0, 0, 0 and **2 of 3,915** (RPM). Attacker bus off in every run (24-31 bus errors). Runs `20261103_140328_DoS`, `140509_Fuzzy`, `140545_gear`, `140628_RPM` (5,000 frames each, in the project's `runs/`, single file, so training frames; attacker version 1) |
| 11-03: the 2 lost RPM frames | 0x002 at 4.1 ms and 0x131 at 5.3 ms after the first attack frame, during the error storm (attacker bus off by about 6.5 ms). **Most likely lost by the ESP32 receiver, not destroyed by the FPGA:** (1) the attacker heard them: its `tracked=` is the normal frames it heard before bus off, 1,716 = 1,708 before the attack + all 8 sent in the storm, both lost frames included (Fuzzy 1,567 = 1,567 + 0, DoS 1,483 = 1,476 + 7, Gear 2,157 = 2,140 + 17 fit the same count); (2) a frame the FPGA flagged would be retransmitted by the sender's controller and arrive late, not be lost; (3) each came right after an error frame, the receiver counted 24 bus errors vs the sender's 21, and the ESP32 TWAI has errata on the frame received after a bus error (the driver's workaround resets the controller; not yet checked against Espressif's errata for this chip revision); (4) the STM32 IPS lost 3 normal frames at the same point of the RPM attack with the same receiver. Not proven yet: see To do |
| 11-03: FPGA IPS, 6 bytes (SW0 up) | Same board, `FEAT_BYTES=6`, generated_p6 (`rf5_d9_p6_time`; run_sim_FPGA.bat 6 all PASS; 82 MHz worst case, 1,930 LEs vs 89 MHz, 1,620 LEs for 8 bytes). **DoS 1,861/1,861 destroyed (two runs), Fuzzy 709/709, Gear 922/922, RPM 1,085/1,085.** Normal frames lost: 0, 0, 0 and **1 of 3,915** (RPM: 0x5F0, 0.95 ms after the first attack frame, in the error storm; the attacker heard only 4 normal frames before bus off this time, so its count cannot confirm it was on the bus). Attacker bus off in every run. Runs `20261103_142404_DoS`, `142503_DoS`, `142545_Fuzzy`, `142625_gear`, `142706_RPM`. The prevention off check was not done for 6 bytes (the 14:24 run had SW0 up) |
| 11-03: FPGA IPS, 5 bytes | `FEAT_BYTES=5`, generated_p5 (`rf5_d9_p5_time`, the STM32's model; run_sim_FPGA.bat 5 all PASS; 89 MHz worst case, 1,618 LEs). Prevention off (SW0 down): **5,000/5,000 delivered, 0 bus errors** (`20261103_143810_DoS`). Prevention on: **DoS 1,861/1,861 destroyed, Fuzzy 709/709, Gear 922/922, RPM 1,085/1,085, normal frames lost 0 in all four** (RPM: the attacker heard all 8 normal frames of the error storm, `tracked=1716`, and so did the receiver). Attacker bus off in every run. Runs `143901_DoS`, `143945_Fuzzy`, `144025_gear`, `144101_RPM`. The RPM normal losses went 2 (8 bytes), 1 (6 bytes), 0 (5 bytes), different IDs each time: one run per model is too few to rank the models on it. Forest sizes (5 trees, depth 9 each): p5 690 splits, p6 874, p8 587; logic elements follow the split count, not the byte count. Board design set to 5 bytes |

