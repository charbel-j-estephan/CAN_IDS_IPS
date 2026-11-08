# Updatable model: results

Every number here was measured by running the tool named. Anything not run yet
is marked **not run**.

## Step 0: feasibility (Quartus 25.1 Lite, 10M50DAF484C7G)

| Question | Answer |
|---|---|
| Does a `.mif` need a "with Memory Initialization" mode? | Yes. In `SINGLE IMAGE` Quartus refused the RAM ("MIF is not supported") and built it from 13,093 LEs. In `SINGLE COMP IMAGE WITH ERAM` it used one M9K |
| Does 256 x 36 simple dual port fit one M9K? | Yes, 9,216 bits |
| UFM available in the ERAM mode | Flash IP generates with 16,384 words of 32 bits: sector 1 words 0-8191, sector 2 words 8192-16383 = **64 KB** |
| Flash timing from the IP at 50 MHz | erase timeout 17,500,000 clocks = 350 ms, write timeout 15,250 clocks = 305 us |

## Phase A: engine, exporter, dataset

Blobs (`python/export_blob.py`):

| Model | Model id | Splits | Blob bytes | Widest level (of 128) |
|---|---|---|---|---|
| 5 bytes | 2ddda531 | 690 | 4,174 | 51 |
| 6 bytes | e7321e76 | 874 | 5,094 | 66 |
| 8 bytes | c0f7e8b5 | 587 | 3,659 | 41 |

Bit exact check (`python/accuracy_engine.py`, Icarus, model written straight into
bank 0): every distinct frame of all 17,558,346 dataset frames, forest class and
final class after the unknown ID rule.

| Model | Distinct frames | Engine vs Python, forest | final |
|---|---|---|---|
| 5 bytes | 675,167 | 0 differ | 0 differ |
| 6 bytes | 761,078 | 0 differ | 0 differ |
| 8 bytes | 977,861 | 0 differ | 0 differ |

The distinct frame counts equal those in `docs/BYTES_COMPARISON.md` for the
hardcoded forest.

Latency and throughput (`tb/latency_tb.v`): **15 clocks** (300 ns at 50 MHz),
100 frames in 100 clocks, read port always yields to frames. Hardcoded: 4 clocks.

Quartus, classifier alone (engine + key table, 8 trees x 12 levels, no CAN and
no update logic; `quartus/engine_only.*`), against the hardcoded forest alone
(5 byte model, `docs/BYTES_COMPARISON.md`):

| | Hardcoded forest | Generic engine |
|---|---|---|
| Logic elements | 1,024 (2 %) | 22,375 (45 %) |
| Registers | 102 | 3,908 |
| Block memory bits | 0 | 888,832 (97 M9K, 53 %) |
| Fmax, slow 85 C | 81.95 MHz | 58.55 MHz |
| Latency | 4 clocks | 15 clocks |

Whole board design numbers come after Phase B.
