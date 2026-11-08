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

## Phase B: SPI, double buffering, CRC, UFM, boot

All from Icarus on the final RTL, using the behavioural flash (`tb/ufm_model.v`).

Dataset through the update port (`accuracy_engine.py --spi`): the blob goes over
the real SPI model, through COMMIT and into flash, then every distinct frame runs
through the classifier.

| Model | Distinct frames | Forest differs | Final differs |
|---|---|---|---|
| 5 bytes | 675,167 | 0 | 0 |
| 6 bytes | 761,078 | 0 | 0 |
| 8 bytes | 977,861 | 0 | 0 |

Update path (`tb/update_tb.v`): **53 checks, 0 failed** (development build) and
**54 checks, 0 failed** (production build, `READ_MODEL_EN = 0`, model read blocked).
Rejected with the old model still active: bad CRC in the body, wrong CRC in COMMIT,
truncated upload, feature index out of range, child pointer past the level, lower
version, wrong device ID, wildcard device ID over SPI, wrong protocol ID, failed
self test, out of order write, outside maintenance mode.
Also: boot ROM on empty flash, two-slot updates, boot from a slot after a power
cycle, rollback floor kept in flash, power cut during a write, power cut during an
erase (both boot the last good model), active slot corrupt (boots the other),
both slots corrupt (boots the ROM default), 3 strike lockout, not cleared by reset,
cleared by power cycle.

Mid-traffic swap 5 to 8 bytes (`tb/core_frames_tb.v -DSWAP_TEST`, `check_swap.py`,
bit level through `can_rx`): 120 frames, 120 verdicts, swap at frame 31, 60 of the
frames are ones where the two models disagree (16 before the swap, 44 after), 0
wrong verdicts, 0 frames with the wrong number of data bytes.

IPS on a simulated bus, model from the boot ROM (`tb/core_frames_tb.v`,
`python/compare_sim.py`): 200 frames, attack frames destroyed 100 of 100 (25 of each
attack), normal frames lost 0 of 100. These are simulated frames, not the bench.

## Hardcoded against updatable: the whole board design

Quartus Prime Lite 25.1, 10M50DAF484C7G, slow 85 C model.

| | Hardcoded (`quartus_board`, 5 bytes) | Updatable (`quartus/updatable_top`) |
|---|---|---|
| Logic elements | 1,618 (3 %) | 29,869 (60 %) |
| Registers | 345 | 7,607 |
| Block memory bits | 0 | 942,080 (56 %) |
| M9K blocks | 0 of 182 | 106 of 182 (58 %, 42 % headroom) |
| UFM | not used | used: 2 slots of 24 KB + 2 journal pages of 8 KB |
| Fmax (clock needed: 50 MHz) | 89.41 MHz | 56.96 MHz |
| Latency, feature ready to verdict | 4 clocks (80 ns) | 15 clocks (300 ns) |

## Phase C: STM32 tool and signer

| Check | Result |
|---|---|
| `stm32_updater` builds (PlatformIO, black F407VE) | Builds. **Not run on hardware** |
| fw 16 unchanged | `mcu/` has no changes; rebuilt `firmware.bin` is byte for byte the bench's fw 16 build (43,312 bytes) |
| `sign_blob.py` | AES-CMAC matches the RFC 4493 vector; a blob signed for one device fails on another; one flipped payload bit is detected |

## Not run: needs the board

| Test | Status |
|---|---|
| UFM behaviour on real silicon (erase, write, read back through the IP) | **Not run** |
| STM32 pushing a model over real wires | **Not run** |
| Four attacks on the ESP32 bench with the SPI-loaded model | **Not run** |
| Unplug the STM32, power cycle, check the model stays | **Not run** |

Steps are in `docs/SPI_WIRING.md`. The flash model matches the Quartus IP's ports,
but only the board can show how the real flash behaves.

## Cross-check against Intel's flash model

`tb/intel_flash_tb.v` runs the flash controller (`rtl/nv/nv_ufm_avmm.v`) against
Intel's own encrypted simulation model of the On-Chip Flash, in Questa: page erase,
word write, read back, both UFM sectors, and a page erase that must leave another
page alone. **All pass.** This checks the controller's register sequence against
Intel's model. The power-cut and slot tests above still use `tb/ufm_model.v`, which
has to be mine: Intel's model does not simulate a power failure.

## Reproducing

`run_sim.bat` ran end to end from an empty `build/` folder on the final RTL: all 8
steps pass. It takes about an hour. `normal_run_data.txt` is extracted from the
dataset zip automatically.

SPI was tested at 2.625 MHz, the speed the STM32 tool uses. The limit is about 4 MHz.
