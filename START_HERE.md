# Start here: where everything is

There are **two folders**. Edit in the first one; run tests from the second.

| Folder | What it is |
|---|---|
| `Desktop\CAN_IDS_IPS` | **The project (git).** All code, the master copy. Change files here. |
| `Desktop\esp32 setup\can_replay` | **The test bench.** A copy of the bench files plus the dataset and your test results. Run the `.bat` files here. |

After changing bench code in the project, copy it to the bench with:

```
python python/sync_bench.py
```

---

## I want to...

| ...do this | Go to |
|---|---|
| Test the bench (ESP32s only) | `can_replay\run_check_esp32.bat` |
| Test the STM32 IDS (detection) | `can_replay\run_check_stm32_IDS.bat` |
| Test the STM32 IPS (prevention) | `can_replay\run_check_stm32_IPS.bat` (add `--file Fuzzy_dataset.csv` etc.) |
| See a test's results | `can_replay\runs\<date>_<file>\report.html` |
| Upload firmware to the STM32 | `can_replay\stm32_ids` (steps in `docs/STM32_WIRING.md`) |
| Wire the ESP32s | `docs/ESP32_WIRING.md` |
| Wire the STM32 | `docs/STM32_WIRING.md` |
| Know what works and what's left | `docs/HANDOVER.md` (status table, one row per version) |
| Run the FPGA simulation | `run_sim_FPGA.bat` |
| Open the FPGA board project | `quartus_board/can_ids_top.qpf` in Quartus |
| Retrain the model | `python/train.py`, then `run_pipeline.sh` |

---

## The project, folder by folder

### `esp32/`: the three ESP32 boards (Arduino)
| Sketch | Board |
|---|---|
| `replay_sender` | **Sender**: plays the normal car traffic |
| `attack_sender` | **Attacker**: plays the attack frames |
| `usb_receiver` | **Receiver**: records everything on the bus |
| `link_sender`, `link_receiver` | Wiring check only, no PC script needed |

### `mcu/`: the STM32
| Folder | What it is |
|---|---|
| `stm32_ids/` | **The STM32 IDS/IPS firmware (fw 16).** This is the one that works. |
| `stm32_ids/src/main.cpp` | Bit timing, the model call, prevention (error flags), USB |
| `stm32_ids/src/can_rx.c` | Decodes CAN frames bit by bit |
| `stm32_ids/src/can_ids_model.h` | The random forest as C code (generated, don't edit) |
| `stm32/`, `platformio_blink/`, `host_test.c` | Old experiments, not used |

### `python/`: PC scripts
**Bench (testing on hardware):**
- `run_check.py`: what the `.bat` files run. Finds the boards, link check, replay.
- `replay_dataset.py`: sends the dataset onto the bus and scores the results.
- `report_html.py`: builds the results page.
- `serial_link.py`, `can_bits.py`: helpers.
- `sync_bench.py`: copies the bench to `can_replay`.
- `check_stm32_model.py`: checks the STM32 decided the same as Python.

**Model (training and export):**
- `dataset.py`: loads the Car Hacking Dataset.
- `train.py`: trains the random forest.
- `export_c.py`: model → C for the STM32.
- `export_verilog.py`: model → Verilog for the FPGA.
- `forest.py`: shared helpers.

**FPGA (simulation and checks):**
- `make_can_vectors.py`, `make_sim_frames.py`: test frames for the simulation.
- `compare_sim.py`: what was sent vs what the FPGA read.
- `fpga_accuracy.py`: FPGA classifier accuracy on the whole dataset.
- `latency.py`: Quartus clock speed → decision time per frame.
- `make_demo_data.py`: a fake dataset, only for checking the tools run.

### `hdl/`: the FPGA Verilog
| File | What it is |
|---|---|
| `can_rx.v` | CAN receiver (the FPGA version of `can_rx.c`) |
| `known_id.v` | The unknown-ID rule |
| `ips_flag.v` | Prevention: the error flag (the FPGA version of M3) |
| `generated_p5/`, `p6/`, `p8/` | The forest as Verilog, one per feature set (generated, don't edit) |
| `*_tb.v` | Testbenches, one per module; `top_tb.v` tests the whole board |
| `vectors/` | Test frames for the testbenches |

### `quartus/` and `quartus_board/`: Quartus projects
- `quartus_board/`: **the full board design** (`can_ids_top.v`) for the DE10 Lite.
- `quartus/`: the classifier alone, to measure its speed.

### `models/`: trained forests
`rf5_d9_p5_time.joblib` is the one in use: 5 trees, depth 9, 5 data bytes.

### `docs/`
| File | What it is |
|---|---|
| `HANDOVER.md` | **Status of everything**, results of every test |
| `ESP32_WIRING.md` | Bench wiring and how to run it |
| `STM32_WIRING.md` | STM32 wiring, upload steps, M3 test |
| `BUILD_GUIDE.md` | How to build the model and FPGA from scratch |
| `BYTES_COMPARISON.md` | Why 5 data bytes (p5) vs 6 or 8 |
| `literature_review.pdf`, `refrences/` | Report material and papers |

### Root files
- `run_check*.bat`: the bench tests (the copies in `can_replay` are the ones you run).
- `run_sim_FPGA.bat`: the whole FPGA simulation.
- `run_pipeline.sh`: train → export to C and Verilog → check they agree.
- `requirements.txt`: Python packages.

### Not your code (downloaded libraries)
`emlearn/`, `FPGA_random_forest/`, `Intrusion-Detection-System-Using-Machine-Learning/`: outside projects used for reference. Don't edit.

---

## The bench folder (`Desktop\esp32 setup\can_replay`)
| Item | What it is |
|---|---|
| `run_check_esp32.bat`, `run_check_stm32_IDS.bat`, `run_check_stm32_IPS.bat` | **Double-click to test** |
| `runs/` | Every test result, newest has the latest date |
| `data/car_hacking/` | The four dataset files |
| `stm32_ids/` | STM32 firmware copy: upload from here |
| `stm32_ids_fw3/` | Old fw 3, kept as a known-good backup |
| `fpga_guide/` | FPGA guide (HTML and PDF) |
| `esp32/`, `python/` | Copies from the project; edit in the project, not here |
