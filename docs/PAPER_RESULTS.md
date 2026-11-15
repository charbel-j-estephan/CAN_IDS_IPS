# Results for the paper: every number, where it comes from

State on 2026-11-15, the last bench day. Each number below has its source: a
run folder, a log or a document. Bench runs are in
`Desktop\esp32 setup\can_replay\runs\<run>` (not in git: they are large; copy
the folders you cite). Simulation logs are in `updatable fpgas/build/` after
`run_sim.bat`.

Read section 7 (limits) before writing the claims.

## 1. System

- Dataset: Car Hacking Dataset (HCRL; Song, Woo, Kim, see `REFERENCES.md`
  [19] and [3]). Four attack files (DoS, Fuzzy, Gear spoofing, RPM spoofing),
  real traffic from a car, 500 kbit/s CAN.
- One model everywhere: random forest, 5 trees, depth 9, trained with a time
  split (`python/train.py --trees 5 --max-depth 9 --payload-bytes 5`),
  features CAN ID, DLC and data bytes 0-4 (`rf5_d9_p5_time`, model ID
  `2ddda531`, 690 splits). Plus an allow list of known CAN IDs (unknown ID =
  Fuzzy).
- Three detectors on the same bus:
  - **FPGA, hardcoded**: the forest compiled into the design (`quartus_board/`).
  - **FPGA, updatable**: the same forest as data in on-chip RAM, loaded over SPI
    from a stamped (AES-CMAC) and scrambled (AES-CTR) blob, kept in the MAX 10's
    user flash, rollback and lockout protected (`updatable fpgas/`).
  - **STM32F407** (168 MHz): CAN decoded bit by bit in software, the forest in C
    (emlearn export), error flag on a GPIO (`mcu/stm32_ids/`, fw 16).
- Prevention: the detector drives an error flag while the attack frame is still
  on the wire, so no node accepts it; the attacker's transmit error counter
  climbs until it goes bus off.
- Bench: three ESP32s (sender of normal traffic, attacker, logging receiver)
  with TJA1050 transceivers, replaying the dataset with its original timing
  (`docs/ESP32_WIRING.md`).

## 2. Classification accuracy (simulation, every frame of the dataset)

Source: `docs/BYTES_COMPARISON.md`. The Verilog (Icarus) and the STM32's C
model were run on every distinct frame; both agree with Python on all of them
(5 bytes: 675,167 distinct frames, 0 differ), so these numbers hold for both
platforms. Scored on the unseen part of the time split (6,539,408 frames):

| | 5 bytes (the bench model) | 6 bytes | 8 bytes |
|---|---|---|---|
| Accuracy | 99.988 % | 99.998 % | 99.998 % |
| Attack frames flagged | 99.980 % | 99.991 % | 99.989 % |
| DoS / Gear / RPM flagged | 100 / 100 / 100 % | 100 / 100 / 100 % | 100 / 100 / 100 % |
| Fuzzy frames missed | 92 | 41 | 50 |
| Normal frames flagged | 720 (0.0119 %) | 66 (0.0011 %) | 55 (0.0009 %) |

669 of the 720 false flags at 5 bytes are one real gear frame that matches the
injected gear frame in bytes 0-4 (it differs in byte 5).

## 3. Hardware cost and timing

| | FPGA hardcoded | FPGA updatable | STM32 |
|---|---|---|---|
| Logic, forest alone | 1,024 LEs (2 %), 81.95 MHz | - | - |
| Logic, whole board design | 1,618 LEs, 89 MHz worst case | 33,253 LEs (67 %), 9,924 registers, 146 of 182 M9K, 56.18 MHz (needs 50) | 42,864 B flash, 29,860 B RAM |
| Decision time after the features are in | 4 clocks, 80 ns | 15 clocks, 300 ns | worst model run per replay 10,069-14,687 cycles, 60-87 µs (`cmax`, 11-15 runs); up to 94 µs in earlier runs (`docs/BYTES_COMPARISON.md`) |

Sources: `docs/BYTES_COMPARISON.md`; `docs/HANDOVER.md` (11-03, 5 bytes);
`updatable fpgas/docs/RESULTS.md` (Stage 5 Quartus table, latency testbench);
`#STAT cmax=` in the 11-15 fw 16 runs.

Time budget: after data byte 4, at least 43 bit times (86 µs at 500 kbit/s)
remain before the end of the frame (`README.md`, timing budget). The FPGA's
decision takes under 1 % of it (under 3 % with the wait for the next bit edge
to start the flag). The STM32's worst model run reaches or passes it; on the
bench no attack verdict came too late (`ilate=0`, `dlate=0` in every run
below). Why the worst runs did not cause a late verdict is not analysed.

The 15-slide deck for the talk follows this document's numbers.

Of the updatable design's 33,253 LEs, the security (AES-128, CMAC, CTR, the
checks) is most of the growth: before the security stage it was 29,869 LEs;
the 40 extra M9K blocks are the AES S-boxes.

## 4. Prevention on the bench

All runs: original timing, attacker on its own board. "Destroyed" = the attack
frame never reached the logging receiver; "normal lost" = a normal frame that
never arrived.

### 4.1 Short runs, 5,000 frames per file

| Detector, date | DoS | Fuzzy | Gear | RPM | Normal lost | Attacker |
|---|---|---|---|---|---|---|
| None (prevention off), 11-15 | 0 / 1,861 | | | | 0 / 3,139 | running, 0 bus errors |
| FPGA hardcoded, 11-03 | 1,861 / 1,861 | 709 / 709 | 922 / 922 | 1,085 / 1,085 | 0, 0, 0, 0 | bus off every run |
| **FPGA updatable, model loaded over SPI, 11-15** | **1,861 / 1,861** | **709 / 709** | **922 / 922** | **1,085 / 1,085** | **0, 0, 0, 0** | bus off every run |
| FPGA updatable after a power cycle, 11-15 | 1,861 / 1,861 | | | 1,085 / 1,085 | 0, 0 | bus off |
| FPGA updatable, RPM repeated 3 times, 11-15 | | | | 1,085 / 1,085 each | 0 in all 3 | bus off |

Runs (11-15): prevention off `20261115_093152_DoS`; FPGA updatable
`093312_DoS`, `093336_Fuzzy`, `093356_gear`, `093416_RPM`; after the power
cycle `095349_DoS`, `095619_RPM`; RPM repeats (with the STM32 as a second
recorder) `094949_RPM`, `095010_RPM`, `095031_RPM`. 11-03: `20261103_143901_DoS`,
`143945_Fuzzy`, `144025_gear`, `144101_RPM` (in the project's `runs/`).

### 4.2 Long runs, 20,000 frames per file, STM32

| Firmware, date | DoS | Fuzzy | Gear | RPM | Normal lost | Late verdicts |
|---|---|---|---|---|---|---|
| fw 16, 11-01 | 5,558 / 5,558 | 2,092 / 2,092 | 3,940 / 3,940 | 4,075 / 4,075 | 0, 0, 0, 3 | 0 |
| fw 16, 11-15 (rerun) | 5,558 / 5,558 | 2,092 / 2,092 | 3,940 / 3,940 | 4,075 / 4,075 | 0, 0, 0, 3 | 0 |
| fw 17, 11-15 | 5,558 / 5,558 | 2,092 / 2,092 | 3,940 / 3,940 | 4,075 / 4,075 | 0, 0, 0, 3 | 0 |

Runs: 11-01 `20261101_191502_DoS`, `191606_Fuzzy`, `191650_gear`,
`191154_RPM`; fw 16 on 11-15 `094139_DoS`, `094224_Fuzzy`, `094254_gear`,
`094326_RPM`; fw 17 `093514_DoS`, `093559_Fuzzy`, `093629_gear`, `093700_RPM`.
Attacker bus off in every run.

**Use fw 16 for the paper** (see 7.3).

## 5. Secure model update (updatable FPGA)

Design: `updatable fpgas/docs/SECURITY_DESIGN.md`, blob layout
`BLOB_FORMAT.md`. Per-board keys derived from a master key with AES-CMAC; the
stamp (CMAC) covers the whole blob; the model bytes are scrambled with AES-CTR;
version floor in flash (no rollback); two flash slots plus a journal (power cut
safe); three bad stamps lock the port until a power cycle; duplicated checks
against glitches.

Verification in simulation (`updatable fpgas/docs/RESULTS.md`):
- AES-128 against FIPS-197 and 1,000 random blocks from Python: 1,001 of 1,001;
  CMAC against RFC 4493: 18 of 18; CTR: 7 of 7.
- Update path testbench: 112 checks (development build) and 111 (production
  build), 0 failed: every refusal (CRC, header, range, version, device, stamp,
  lockout), power cuts during erase and write, glitch cases.
- Every dataset frame through the model loaded over SPI, plain and scrambled,
  5 / 6 / 8 bytes: 0 differ from Python.

On the board, 11-14/15 (`updatable fpgas/docs/RESULTS.md`, home test): update
accepted into slot A then slot B; older version refused; three wrong-key blobs
refused and the port locked (a reset does not unlock it, a power cycle does);
the model kept through power cycles and boots from flash with no PC attached.

## 6. Figures you can make from the data

- Prevention, per attack, per detector: the tables in section 4.
- Timing: `frames.csv` in each run has `received_us`, `timing_error_us` and
  (STM32 runs) `ids_decided_after_features_us`; `report.html` already plots
  them.
- Resources: section 3.
- Each `report.html` is a ready results page; screenshots of
  `20261115_100339_DoS` (no protection) next to `20261115_100416_DoS` (FPGA)
  show the effect in one picture.

## 7. Limits: say these in the paper

1. **The bench replays the first 5,000 or 20,000 frames of each file, and those
   frames are in the training part of the time split.** The bench shows the
   prevention works in real time on a real bus; the accuracy on unseen frames
   is the simulation result in section 2 (6.5 million frames). A held-out bench
   run needs attacker version 2 and `--file mixed` (not done).
2. **The 3 lost normal RPM frames with the STM32 IPS.** Always the same three
   frames (0x153, 0x130, 0x43F; indices 1715, 1717, 1721, at the start of the
   attack), with fw 16 on 11-01 and 11-15 and with fw 17. The FPGA with the
   same 5-byte model lost none in 7 RPM runs (11-03 once; 11-15 six times:
   `093416`, `094949`, `095010`, `095031`, `095619`, `100445`). On 11-03 the
   hardcoded FPGA with the 8 and 6 byte models lost 2 and 1 normal RPM frames
   (other IDs, during the error storm; `docs/HANDOVER.md` argues the ESP32
   receiver dropped them). The sender reports no failed transmissions in any
   of these runs. Not explained: report them as lost.
3. **fw 17** (two IPS race fixes from a code review) gives the same results but
   its bit samples are late about 35 times more often than fw 16's (`nlate`
   24,557-29,978 vs 653-898, same bench, same day). Open; fw 16 is the
   firmware on the board and the one to report.
4. The updatable design was not run on the bench with the 6 or 8 byte model
   (only in simulation).
5. Bench at 500 kbit/s with four nodes and short wires, not a car.

## 8. Where everything is

| What | Where |
|---|---|
| Status of every firmware and design version | `docs/HANDOVER.md` |
| Updatable FPGA: all results | `updatable fpgas/docs/RESULTS.md` |
| Security design | `updatable fpgas/docs/SECURITY_DESIGN.md` |
| Live demo | `docs/DEMO.md`, `run_demo.bat` |
| Literature review, references | `docs/literature_review.pdf`, `REFERENCES.md` |
| Bench runs (not in git) | `Desktop\esp32 setup\can_replay\runs\` |
