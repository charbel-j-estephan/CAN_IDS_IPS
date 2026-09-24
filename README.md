# CAN bus random forest IDS on FPGA and MCU

One random forest, trained on the HCRL Car Hacking Dataset, built three ways:

1. **FPGA only**: pipelined Verilog on the DE10 Lite (MAX 10 10M50DAF484C7G)
2. **MCU only**: emlearn C code on the STM32F407
3. **Combined**: both together

Python generates the Verilog and the C from the same trained model. It also
generates test vectors, so you can prove the FPGA logic, the C code and
Python give the same class for every frame before you touch hardware.

```
python/            dataset loader, training, Verilog and C exporters, latency math
hdl/rf_ids_tb.v    Icarus Verilog testbench
quartus/           Quartus Prime Lite project for the DE10 Lite
mcu/host_test.c    runs the C model on your PC against the test vectors
mcu/stm32/         drop in STM32 code that times each classification
run_pipeline.sh    train, export, simulate and check in one command
```

See [REFERENCES.md](REFERENCES.md) for every paper, dataset and tool used.

The three upstream repos from the original plan are git submodules here, for
reference only. The pipeline does not import them (see "Why not use the
upstream repos directly" below).

## 1. Get the code

Install [Git](https://git-scm.com) (on Windows keep the installer defaults, it
gives you Git Bash). Then in Git Bash:

```bash
cd ~/Desktop
git clone --recursive https://github.com/charbel-j-estephan/CAN_IDS_IPS.git
cd CAN_IDS_IPS
```

`--recursive` also downloads the three submodules. If you already cloned
without it, run `git submodule update --init`.

## 2. Install the tools

| Tool | What for | Get it |
|---|---|---|
| Python 3.9+ | training and exporting | python.org, tick "Add Python to PATH" |
| Python packages | | `pip install -r requirements.txt` |
| Icarus Verilog + GTKWave | simulate the Verilog | Windows installer from bleyer.org/icarus (includes GTKWave) |
| Quartus Prime Lite | synthesis and timing | intel.com, pick MAX 10 device support. Several GB, start early |
| STM32CubeIDE | MCU build | st.com |
| gcc (optional) | run the C model on your PC | MSYS2 on Windows, already on Linux and macOS |

GHDL, suggested in the first draft of this plan, only reads VHDL. It cannot
simulate the Verilog this project produces, so use Icarus Verilog.

Check your setup with fake data before you download anything:

```bash
./run_pipeline.sh demo
```

You should see two PASS lines at the end. The demo data is synthetic, so
ignore its accuracy.

## 3. Get the dataset

Download the Car Hacking Dataset from
https://ocslab.hksecurity.net/Datasets/car-hacking-dataset and unzip it into
`data/car_hacking/`. You need these five files:

```
DoS_dataset.csv  Fuzzy_dataset.csv  gear_dataset.csv  RPM_dataset.csv  normal_run_data.txt
```

Each frame becomes 10 integer features: CAN ID, DLC and the 8 data bytes.
Classes are Normal, DoS, Fuzzy, Gear_spoof and RPM_spoof. Attack files mark
real traffic with R and injected frames with T, and the loader labels them
accordingly.

## 4. Train

```bash
python python/train.py --data data/car_hacking --trees 100 --max-depth 10
```

This prints accuracy, a per class report and the forest size, then saves
`models/can_ids_forest.joblib` along with the held out test set. The full
dataset is about 17 million frames. Add `--limit-per-file 500000` for quick
experiments.

Two knobs control hardware size: `--trees` (n_estimators) and `--max-depth`.
Depth matters most, because each extra level can double a tree's node count.
`--max-depth 0` means unlimited and gives a huge design, so avoid it.

The script prints two accuracies. scikit learn averages probabilities (soft
vote). The FPGA and emlearn count one vote per tree (hard vote). Report the
hard vote number, since that is what the hardware computes.

## 5. Export to Verilog and simulate

```bash
python python/export_verilog.py
mkdir -p build
iverilog -g2005 -I hdl/generated -o build/rf_ids_sim hdl/rf_ids_tb.v hdl/generated/rf_ids.v
vvp build/rf_ids_sim          # add +vcd to write build/rf_ids.vcd
gtkwave build/rf_ids.vcd      # optional, look at the waveforms
```

`hdl/generated/rf_ids.v` is a 4 stage pipeline:

| Stage | Work |
|---|---|
| 1 | register the incoming frame |
| 2 | every tree runs in parallel as comparators, each tree's class is registered |
| 3 | count votes per class |
| 4 | pick the class with the most votes, ties go to the lowest index |

It accepts a new frame every clock and answers 4 clocks later. The testbench
streams 2000 test frames through it and prints PASS only when every result
matches Python.

Ports:

```verilog
input         clk, rst, in_valid
input  [10:0] can_id
input  [3:0]  dlc
input  [63:0] data        // byte 0 in data[63:56], bytes past DLC = 0
output        out_valid
output [2:0]  class_out   // 0 Normal, 1 DoS, 2 Fuzzy, 3 Gear_spoof, 4 RPM_spoof
```

## 6. Synthesize in Quartus

Open `quartus/can_ids.qpf` in Quartus Prime Lite. The project already targets
the 10M50DAF484C7G, uses `hdl/generated/rf_ids.v` as the top level and
constrains the clock to the board's 50 MHz oscillator. Click Processing, Start
Compilation. Or from a terminal:

```bash
cd quartus
quartus_sh --flow compile can_ids
```

The data ports are virtual pins, since in the final design they connect to
on chip logic, not to board pins. That keeps pin delays out of your Fmax.

Every time you retrain, rerun `export_verilog.py` and recompile. Quartus
picks up the new file automatically.

## 7. Read the reports

Two numbers matter.

**Fit.** Compilation Report, Flow Summary, "Total logic elements". The
10M50 has 49,760. If the design does not fit, lower `--max-depth` first,
then `--trees`, then retrain and re-export. For reference, 100 trees at depth
10 on the demo data came out near 2,900 LUTs and 410 flip flops in a Yosys
test synthesis. Real data grows bigger trees, so check your own number.

**Speed.** Compilation Report, Timing Analyzer, Slow 1200mV 85C Model, Fmax
Summary. Use the slow model, it is the guaranteed worst case. Then:

```bash
python python/latency.py --fmax 120.5 --budget-us 86
```

Latency is 4 clocks divided by the clock frequency. At the board's 50 MHz that
is 80 ns, about a thousand times under an 86 us budget. So for this design
the tree count mostly decides whether it fits, not whether it is fast enough.
The real delay in the FPGA build is getting the frame off the bus: an 8 byte
CAN frame takes about 222 us to transmit at 500 kbit/s. Measure latency from
the end of the frame, and write down where you start the clock in your
report.

None of this needs the board. Quartus timing comes from place and route. Use
the DE10 Lite at the end to confirm the real behavior matches.

## 8. Export to C and run on the STM32

```bash
python python/export_c.py
gcc -O2 -I mcu/generated -o build/host_test mcu/host_test.c   # optional PC check
./build/host_test hdl/generated/vectors.hex
```

The PC check uses the same vectors as the Verilog testbench. Two PASS results
mean the FPGA and the STM32 run the identical classifier, which is what makes
your three way comparison fair.

In STM32CubeIDE:

1. Copy `mcu/generated/can_ids_model.h`, `mcu/stm32/ids_timing.c` and
   `mcu/stm32/ids_timing.h` into your project (`Core/Inc` and `Core/Src`).
2. Call `ids_timing_init()` once after `SystemClock_Config()`.
3. In your CAN receive callback call `ids_classify(hdr.StdId, hdr.DLC, data)`.
   It returns the class and the CPU cycles the forest took.
4. Convert with `ids_cycles_to_us()` and log the **worst case** over many
   frames, not the average.

Timing uses the Cortex M4 DWT cycle counter, which counts every CPU clock
(168 MHz on the F407), so you need no timer peripheral setup.

## Why not use the upstream repos directly

The first plan used three repos for the three jobs. Each has a problem for
this project:

* **Western-OC2-Lab/Intrusion-Detection-System-Using-Machine-Learning**. The
  notebooks load CICIDS2017 CSV columns. They do not read the Car Hacking
  files without rewriting the preprocessing, and they train models that never
  reach hardware. `python/dataset.py` and `python/train.py` replace them. The
  notebooks and papers stay useful as a reference for your literature review.
* **johnbensnyder/FPGA_random_forest** (`skhdl_64.py`). It averages each
  leaf's class 0 value, which acts as a regressor, not a classifier, so it
  cannot output an attack class. It reads `n_features_`, which scikit learn
  removed in 1.2, so it crashes on current versions. It targets a Xilinx board
  with a 9600 baud UART, and it evaluates all trees inside one clock with no
  pipeline, which hurts Fmax. Its README also reports wrong predictions it
  never fixed. `python/export_verilog.py` replaces it.
* **emlearn**. It works and the pipeline uses it through pip. Two issues are
  handled in `python/export_c.py`. scikit learn splits with `x <= t` and
  emlearn writes `x < t`, so a frame value equal to a whole number threshold
  goes the wrong way. emlearn's default int16 mode also rounds a threshold
  like 157.5 down to 157. On a test model with 8 trees this changed 156 of
  2000 predictions. The exporter fixes both by moving each threshold to
  floor(t) + 0.5 and exporting float thresholds.

## Limits to mention in your report

* Features are per frame only. Timing features, like the gap since the last
  frame with the same ID, catch DoS and fuzzing better, but they need a per ID
  timestamp table on the FPGA. That is a good next step.
* A random train and test split of frames from the same drive overstates
  accuracy, because neighboring frames look alike. Say so, or split by time.
