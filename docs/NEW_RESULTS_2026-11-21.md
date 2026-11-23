# New results for the paper revision (21 Nov 2026)

Everything here was run off the bench, on the held out part of the time split,
with the same model as the paper. The bench tests that still need you are at
the end, with what to look for in each.

## 0. Same model as the paper

Retrained `train.py --trees 5 --max-depth 9 --payload-bytes 5` on the four
attack files plus normal_run_data.txt. It gives exactly the paper's numbers.
6,539,408 test frames, 99.988 % accuracy, 720 normal frames flagged, 690
splits. The 6 byte model also matches (66 false flags). So every number below
is for the bench model.

Note. Your laptop's data\car_hacking has no normal_run_data.txt any more. Without
it you get 6,341,633 test frames and 68,948 false flags, so keep that file in
the folder for any rerun.

## 1. Ablation, allow list against forest

| Detector | DoS | Fuzzy | Gear | RPM | Normal flagged |
|---|---|---|---|---|---|
| Allow list alone | 100 % | 98.103 % | 0 % | 0 % | 0 |
| Forest alone | 100 % | 95.176 % | 100 % | 100 % | 720 |
| Both (bench) | 100 % | 99.906 % | 100 % | 100 % | 720 |

The allow list alone stops DoS (ID 0x000 is not a car ID) and most fuzzing.
The forest is what catches both spoofing attacks, and it adds 1.8 points on
fuzzing. Reviewers will ask for this table.

## 2. Short frames

1.1 % of test frames are shorter than 8 bytes, and none of them is an attack.

| DLC | Frames | IDs |
|---|---|---|
| 2 | 64,253 | 0x5F0 |
| 5 | 10,169 | 0x2B0, 0x6AA |
| 8 | 6,464,986 | all others, every attack frame |

For a frame with DLC of 5 or less the features are complete at the end of the
data field, so only 19 bit times (38 µs) are left, not 86 µs. The FPGA (300 ns)
has no problem. The STM32's worst model run (60 to 94 µs) would miss that
window, so an attacker using short frames would get through the STM32. Say
this in the timing section.

## 3. False flags can push a real ECU to bus off

A destroyed genuine frame is retransmitted unchanged, the model flags it
again, and after about 32 tries the real sender goes bus off. So each false flag
is a possible bus off of a real ECU, not one lost frame.

Where the 720 false flags are.

* 714 are in normal_run_data.txt, 6 in the attack files. The bench only replays
  attack files, so it never saw them. That's why the bench lost almost nothing.
* 669 are on ID 0x43F (the gear frame), 45 on 0x0A0, 6 spread over others.
* The 0x43F ones are periodic, about every 10 ms, in one stretch. That is one
  ECU that would be pushed off again and again.

What does not fix it. A burst rule ("destroy only after M attack verdicts on
the same ID within W ms") barely helps, because the 0x43F false flags are
themselves periodic. It also ruins fuzzing, which spreads over random IDs
(M = 2 lets 98 % of fuzzing frames through).

What does fix it. **Destroy only on a unanimous vote**, and treat 3 or 4 of 5
votes as an alert only.

| Rule | Model | Normal destroyed | DoS | Fuzzy | Gear | RPM |
|---|---|---|---|---|---|---|
| Majority (today) | 5 bytes | 720 | 100 % | 99.906 % | 100 % | 100 % |
| At least 4 of 5 | 5 bytes | 670 | 100 % | 99.799 % | 100 % | 100 % |
| **5 of 5** | 5 bytes | **0** | 100 % | 99.485 % | 100 % | 100 % |
| Majority (today) | 6 bytes | 66 | 100 % | 99.958 % | 100 % | 100 % |
| **5 of 5** | 6 bytes | **0** | 100 % | 99.566 % | 100 % | 100 % |

Every false flag has 3 or 4 votes. 95 % of true attack flags have 5. The cost
is 0.4 points of fuzzing, frames the allow list cannot catch because they
carry a real car ID.

This is a very small hardware change, one comparison in the vote stage. For
the updatable story it's even better as a field in the model file (minimum
votes to destroy), so it can be changed in the field like the trees.

Files: docs\new_results_data (analysis.json, votes_rule.json, fp_rows.json), made by the scripts in docs\analysis_scripts.

## 4. What the existing bench logs already show

From the runs saved in CAN_IDS_IPS\runs (3 to 15 Nov).

**The attacker is stopped by its first attack frame.** In every RPM run the
first attack frame is frame 1708. The attacker goes bus off during that
frame's retries (about 32 tries), and stays off (single file runs never ask it to recover). Its
report shows tracked=1716, i.e. it stopped hearing the bus right there. So of
the 1,085 "destroyed" RPM frames, the detector really destroyed one frame (and
its ~32 retries). The other 1,084 were never sent. Table IV should say
"blocked" and give both numbers, destroyed and suppressed by bus off. The mixed
runs (attacker v2 recovers at every block) are the honest test, see section 6.

**Lost normal frames happen during that retry burst.** Two runs lost normal
frames, and all of them are frames 1710 to 1718, right after the first attack
frame.

| Run | Lost normal frames |
|---|---|
| 20261103_140628_RPM | 1714 (ID 0x002), 1718 (0x131) |
| 20261103_142706_RPM | 1710 (0x5F0) |
| 20261103_144101_RPM | none |

The paper says the FPGA lost no normal frame in seven RPM runs, so check which
detector and model these two runs used. Either way, the losses sit inside the
few milliseconds where the bus is full of error flags and retries. The STM32's
"3 frames at the start of every RPM run" is very likely the same thing. This
gives the paper a concrete cause instead of "not determined".

## 5. ASIC synthesis (Yosys, SkyWater 130 nm, typical corner)

Scripts and cell reports are in docs\asic_synth.

Standard cell area after synthesis only. No place and route and no timing
sign off. Memories are left as black boxes, because on an ASIC they become
SRAM and ROM macros.

| Block | Cell area |
|---|---|
| Hardcoded forest (rf_ids, 5 bytes) | 0.010 mm² |
| Updatable tree engine (rf_classifier) | 0.431 mm² |
| One AES-128 core | 0.068 mm² |
| Whole updatable core (engine, CAN front end, SPI, update, AES, CMAC, CTR) | 0.688 mm² |

Plus memories for the updatable core: 96 tree RAMs of 256 × 36 bits (two banks
of 8 trees × 12 levels), about 885 kbit (108 KB) of SRAM, plus the ID table,
a 512 byte metadata RAM and the boot ROM.

So updatability costs about 70 times the logic of the fixed forest, plus
~108 KB of SRAM. That's still under 1 mm² of logic in an old 130 nm process.
Most of the engine area is the feature select and comparator per level, sized
for the maximum of 8 trees, 12 levels and 16 features. A smaller maximum
shrinks it. Report it as a design choice for headroom.

## 6. Bench tests to run (STM32 only, nothing to flash)

Double click `run_paper_tests.bat` in the repo root. It runs these in a row,
about 30 to 40 minutes.

1. **Held out mixed runs with prevention on**, 2 × 40,000 frames. Blocks from
   all four files, only from the part the model never saw, attacker recovering
   at every block. This replaces the training frame bench results. Look at
   attack frames lost per class, normal frames lost, and how many times the
   attacker went bus off and recovered (busoffs, recovered in the attacker
   report).
2. **Prevention off** for Fuzzy, gear and RPM, to fill Table IV.
3. **False flag test** (`python\fp_test.py`, data in `data\fp_test`). 20,000
   genuine frames from the held out part of normal_run_data.txt, with 699 that
   the 5 byte model flags. No attacker. Look at the sender's state and tec in
   summary.json. If it says busoff, that is section 3 proven on hardware.
4. **One long held out run**, 200,000 frames.

## 7. FPGA to do list (when you have a board)

1. **Held out mixed run, FPGA as IPS.** Same as the STM32 one,
   `run_check.py --ips --file mixed --n 40000` with the FPGA on the bus.
2. **Lost frames.** An FPGA IPS run on RPM_dataset.csv with the STM32 as
   `--listen` recorder. It shows whether the normal frames lost in the retry
   burst were lost on the bus or only missed by the ESP32 receiver.
3. **Unanimous vote rule on hardware.** Change the vote stage to destroy only
   on 5 of 5 votes (3 or 4 votes give an alert only), build a new bitstream,
   then rerun 1 and the false flag test (`python\fp_test.py`). Best done as a
   field in the model file, so it updates with the trees.
4. **Live model update on the bench.** Push the 6 byte model through the
   secure SPI path with no resynthesis while traffic runs, and show the false
   flags drop. This is the headline experiment of the paper.
5. **Quartus Power Analyzer.** Once for the hardcoded design and once for the
   updatable one, with a VCD from a simulation with real CAN traffic. Report
   core dynamic and static power, labeled as estimated.
6. **Power on hardware, optional.** Only if the university lab has a bench
   supply or USB power meter.

## 8. STM32 runs (esp32 setup\can_replay\runs), added 22 Nov

**The STM32 only had to destroy 22 to 32 frames per run.** In every STM32 IPS
run of 1 and 15 Nov the IPS counter reads rej=32 for Fuzzy, gear and RPM and
rej=21 to 22 for DoS (one early RPM run on 1 Nov reads 88). The attacker
always ends in state=busoff with tec=128 and buserr=31 (21 to 22 for DoS).
So the 1,085 or 4,075 "destroyed" attack frames in Table IV are really one
attack frame's retries destroyed, then an attacker that is off the bus. Say
"blocked", and give the number of flags actually sent.

**No flag was ever late.** Every IPS run shows ilate=0 and dlate=0, and all
flags were pushed to a valid start bit (fpush equals fstart).

**That explains the timing paradox.** The paper's worst model run applies to
every frame, but only 22 to 32 frames per run ever needed a flag in time. The
one frame by frame timing log (20261101_190059_RPM, IDS mode, 20,000 frames)
shows the decision came 47.9 µs after the features at the median, 63.9 µs at
p99 and 65.8 µs at most, with at least 12.0 µs left before the frame ended
(30.4 µs at p1). No frame lacked a decision by the frame end. Use these
numbers in Table III, and drop "we did not analyze why".

**The 3 lost normal frames come from the same burst.** In every IPS run the
genuine sender also logs 15 to 31 bus errors, because its frames meet the
error flags during the retry burst. Only RPM runs lose normal frames (3 per
STM32 run, 0 to 2 in the FPGA runs), and in the runs I could check they are the
frames sent right after the first attack frame. A --listen run with the FPGA
as IPS and the STM32 as recorder will show whether they were lost on the bus
or only missed by the ESP32 receiver.

**What changes with a recovering attacker.** The held out mixed runs in
run_paper_tests.bat make the attacker come back at every block, so the
detector has to keep destroying frames for the whole run. Those runs are the
real test of the STM32 timing.

## 9. Power, estimated (not measured)

**STM32F407 at 168 MHz.** From the datasheet table for run mode with code from
flash and the ART accelerator on, typical at 25 °C. The chip draws about
40 mA with all peripherals off and about 87 mA with all peripherals on. Your
firmware uses only a few (timers, USB, GPIO), so the real figure sits between
the two. At 3.3 V that is about **130 to 290 mW for the chip**, drawn the whole
time because the CPU never sleeps. One model run (48 µs median, 66 µs max from
the timing log) costs roughly 6 to 19 µJ. This is the chip alone, not the
board's regulator, LEDs or USB bridge.

**ASIC (SKY130).** Area only, see section 5. Yosys gives no power figure, so
the paper should say ASIC power is future work.

**FPGA.** Still to do on your laptop with the Quartus Power Analyzer (Processing,
then Power Analyzer Tool), once for the hardcoded design and once for the
updatable design. Give it a VCD from a simulation with real CAN traffic so the
switching activity is realistic, and report the core dynamic and static power.

## 10. Strict vote mode in the FPGA (added 22 Nov)

New switch SW2 on the updatable design. Up = the IPS destroys a frame only on a
strong verdict (all enabled trees voted the winning class, or the key table
rule fired on an unknown ID). A weak attack verdict (3 or 4 of 5) stays an
alert, LEDR1 without LEDR2. Files changed: rtl/engine/rf_engine.v
(out_unanimous), rtl/engine/rf_classifier.v (out_strong),
rtl/top/updatable_core.v (strict input, ips_verdict), rtl/top/updatable_top.v
(SW2), the four testbenches that use these, python/accuracy_engine.py (checks
the strong flag too), python/paths.py (Icarus from PATH off Windows).

Simulation, all in Icarus 12 with the real blobs.

| Check | Result |
|---|---|
| Every distinct dataset frame, engine vs Python, forest, final and strong | 5 bytes 675,167 frames, 6 bytes 761,078 frames, **0 differ** |
| Distinct frames with an attack verdict that are weak | 3,243 of 492,632 (5 bytes), 2,016 of 491,816 (6 bytes) |
| Latency | 15 clocks, unchanged |
| Update path testbench | 112 checks, 0 failed |
| Whole design on a bus, 200 dataset frames (100 attacks) | majority 100 flags, strict 100 flags, every attack destroyed |
| Whole design on a bus, 300 genuine frames from data\fp_test | majority **5 flags** (genuine frames destroyed), strict **0 flags** |

The FPGA test day (docs\FPGA_TEST_GUIDE.html, fpga_tests.bat) measures this
on the bench as tests T3 and T4b.
