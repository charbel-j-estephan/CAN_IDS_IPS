# 5, 6 or 8 data bytes: the same forest on the FPGA and the STM32

Three models that differ only in how many data bytes they read. Everything
else is identical: 5 trees, depth 9, the time split (test = the last 20 % of
each file), seed 0, the 38-ID unknown ID rule.

    py -3.13 python/train.py --data data/car_hacking --trees 5 --max-depth 9 --payload-bytes N --split time --out models/rf5_d9_pN_time.joblib

| Bytes | Model file | Model id |
|---|---|---|
| 5 | `models/rf5_d9_p5_time.joblib` | `rf5-d9-p5-2ddda531` (the original; retraining with the command above gives the same id) |
| 6 | `models/rf5_d9_p6_time.joblib` | `rf5-d9-p6-e7321e76` |
| 8 | `models/rf5_d9_p8_time.joblib` | `rf5-d9-p8-c0f7e8b5` |

## Where each copy lives

| | 5 bytes | 6 bytes | 8 bytes |
|---|---|---|---|
| FPGA forest | `hdl/generated_p5/rf_ids.v` | `hdl/generated_p6/rf_ids.v` | `hdl/generated_p8/rf_ids.v` |
| FPGA receiver | `can_rx #(.FEAT_BYTES(5))` | `FEAT_BYTES(6)` | `FEAT_BYTES(8)` |
| STM32 model | `mcu/stm32_ids/src/can_ids_model.h` | `can_ids_model_p6.h` | `can_ids_model_p8.h` |
| STM32 build | `pio run -e black_f407ve_dfu` | `-e black_f407ve_dfu_p6` | `-e black_f407ve_dfu_p8` |
| Accuracy report | `build/accuracy/rf5_d9_p5_time/` | `.../rf5_d9_p6_time/` | `.../rf5_d9_p8_time/` |

`known_id.v` is the same for all three (same 38 IDs).

## Classification: every frame of the dataset (17 558 346 frames)

`python/fpga_accuracy.py` ran every distinct frame through the Verilog
(`rf_ids.v` + `known_id.v` in Icarus) and through the STM32's C model (the
exported header, evaluated as C does: float compares, one vote per tree, the
2048-bit ID table). Both agreed with Python on every distinct frame, so the
numbers below hold for **both platforms**:

| Bytes | Distinct frames | FPGA vs Python | STM32 C vs Python |
|---|---|---|---|
| 5 | 675 167 | 0 differ | 0 differ |
| 6 | 761 078 | 0 differ | 0 differ |
| 8 | 977 861 | 0 differ | 0 differ |

Unseen frames only (6 539 408, the fair score):

| | 5 bytes | 6 bytes | 8 bytes |
|---|---|---|---|
| Accuracy | 99.988 % | **99.998 %** | **99.998 %** |
| Attack frames flagged | 99.980 % | **99.991 %** | 99.989 % |
| DoS / Gear / RPM flagged | 100 / 100 / 100 % | 100 / 100 / 100 % | 100 / 100 / 100 % |
| Fuzzy frames missed | 92 | **41** | 50 |
| Normal frames flagged (false blocks) | 720 (0.0119 %) | 66 (0.0011 %) | **55 (0.0009 %)** |
| of which normal recording, 43F as Gear | 669 | 49 | 49 |

The 669 false Gear verdicts at 5 bytes are a real gear frame (`43F 01 45 60
FF 6C ..`) that differs from the injected one (`43F 01 45 60 FF 6B 00 00 00`)
only in byte 4 within the first 5 bytes; byte 5 separates most of them. The
49 left at 6 and 8 bytes have `00 00 00` in bytes 5-7 like the attack, so
reading bytes 6-7 does not help them.

## Timing: what the IPS needs

The error flag must start before the end of frame field. After the last byte
the model reads, at least `19 + 8 x (8 - N)` bit times are left (more with
stuff bits), 2 us each at 500 kbit/s:

| | 5 bytes | 6 bytes | 8 bytes |
|---|---|---|---|
| Time left after the last byte read | 86 us | 70 us | 38 us |
| FPGA decision | 80 ns (4 clocks) + up to 2 us to the next bit edge | same | same |
| STM32 model run (earlier IPS runs, `cmax`) | 62-94 us worst case | not measured yet | not measured yet |

On the FPGA every verdict is in time with all three models. On the STM32 the
worst case already exceeds 86 us; the 6 and 8 byte builds need bench runs to
measure `cmax`, `ilate` and `dlate` (below).

## Hardware cost

FPGA, Quartus Prime Lite 25.1, MAX 10 10M50DAF484C7G, forest alone
(`build/quartus_pN/`, virtual pins):

| | 5 bytes | 6 bytes | 8 bytes |
|---|---|---|---|
| Forest nodes | 1385 | 1753 | 1179 |
| Logic elements | 1 024 (2 %) | 1 300 (3 %) | 960 (2 %) |
| Registers | 102 | 110 | 126 |
| Fmax (slow 85 C) | 81.95 MHz | 88.74 MHz | 91.53 MHz |

STM32F407 firmware (`pio run`, model in RAM):

| | 5 bytes | 6 bytes | 8 bytes |
|---|---|---|---|
| Flash | 42 864 B | 47 056 B | 42 628 B |
| RAM | 29 860 B | 34 072 B | 29 660 B |

The 5-byte firmware is byte for byte the fw 16 build (same `firmware.bin`).

## Reproduce

From the repo root:

    py -3.13 python/fpga_accuracy.py --model models/rf5_d9_p5_time.joblib --rf hdl/generated_p5/rf_ids.v --c-header mcu/stm32_ids/src/can_ids_model.h
    py -3.13 python/fpga_accuracy.py --model models/rf5_d9_p6_time.joblib --rf hdl/generated_p6/rf_ids.v --c-header mcu/stm32_ids/src/can_ids_model_p6.h
    py -3.13 python/fpga_accuracy.py --model models/rf5_d9_p8_time.joblib --rf hdl/generated_p8/rf_ids.v --c-header mcu/stm32_ids/src/can_ids_model_p8.h
    run_sim.bat 5        (and 6, 8: the CAN bit level, can_rx + forest on 1000 frames)

## Still to measure on the bench (STM32)

For each byte count, from `Desktop\esp32 setup\can_replay\stm32_ids`:

1. BOOT0 to 3V3, RST, then `pio run -e black_f407ve_dfu_p6 -t upload`
   (`black_f407ve_dfu` for 5 bytes, `_p8` for 8), BOOT0 back to GND, RST.
2. `run_check_stm32.bat`: the STM32 must decode 100 %; `#STAT` shows
   `id=rf5-d9-p6-...`, and `cmax`, `dlate`.
3. `run_check_ips.bat` (all three ESP32s): attack frames destroyed, normal
   frames lost, `ilate` (verdicts too late), `cmax`.
4. `py -3.13 python/check_stm32_model.py --run <run folder> --model models/rf5_d9_p6_time.joblib`

The FPGA side of the bus-level IPS needs `ips_flag.v` and a board.
