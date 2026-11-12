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

## Audit follow-up, Stages 1-5 (2026-11-12)

The fixes and additions from the code audit, all in simulation. Nothing in this
section has run on the board.

### Stages 1-4: bugs and missing tests

| Check | Result |
|---|---|
| Flash controller when the flash stops answering (`tb/nv_timeout_tb.v`): stuck erase, stuck write, stuck read | **23 checks, 0 failed**. The same test on the old controller fails 5 (stuck busy, sector never locked again, request lines left high) |
| Flash controller against Intel's model (Questa), after that fix | **All pass** |
| IPS rate limit (`tb/ips_limit_tb.v`): off by default, 2 per window when on | **7 checks, 0 failed** |
| Update path before Stage 5 (`tb/update_tb.v`) | 74 checks, 0 failed (development), 73 (production). New: 5 data byte minimum, level 12 READ refused, journal page full and rolled over, no model at all repaired over SPI |
| Mid-traffic swap, the new model now over the real SPI pins (was forced into the decoder) | 120 frames, 120 verdicts, swap at frame 72, 36 disagreeing frames before it and 24 after, 0 wrong |
| STM32 fw 17 (two IPS races) | Builds, 43,352 bytes. Bit interrupt saves the same registers as fw 16 (r4-r8, lr) and is 4 bytes smaller. **Not run on hardware** |
| `stm32_updater` with the 64 character line limit | Builds. **Not run on hardware** |

### Stage 5: stamp, scrambling, glitch hardening

| Check | Result |
|---|---|
| AES-128 (`tb/aes_tb.v`): FIPS-197 C.1 and 1,000 random blocks vs Python `cryptography` | **1,001 of 1,001**, 21 clocks per block |
| Seal checker, AES-CMAC (`tb/cmac_tb.v`): RFC 4493 at 16, 40, 64 bytes, the three real blob lengths, 11 random lengths, restart mid message | **18 of 18**. RFC 4493's 0 byte case is checked in Python only (a blob is never shorter than 708 bytes) |
| Keystream, AES-CTR (`tb/ctr_tb.v`): blocks 0-2 for two keys and IVs, restart | **7 of 7** |
| Update path (`tb/update_tb.v`) | **95 checks, 0 failed** (development), **94, 0 failed** (production) |
| ... stamp missing, one stamp bit wrong, a threshold changed without the key, another master key | each refused with `E_MAC`, old model kept |
| ... scrambled blob with its flag cleared | refused before the stamp (`E_RANGE`) |
| ... first stamp compare forced to "yes" | still `E_MAC` |
| ... the two lockout copies forced to disagree | port locked; a power cycle clears it |
| ... three wrong stamps | locked; KEY[0] does not clear it, a power cycle does |
| ... an unstamped rulebook already in flash | not booted: the ROM model runs |
| ... scrambled rulebook | accepted, boots again after a power cycle; of 1,000 node bytes in flash, 1,000 equal the scrambled blob and 3 the plain one (chance) |
| ... empty flash | the ROM model still boots without a stamp |
| IPS on a simulated bus, Stage 5 design | attack frames destroyed 100 of 100, normal frames lost 0 of 100 |
| Mid-traffic swap, Stage 5 design | as above: 120 of 120, 0 wrong |
| Every dataset frame over SPI, stamped plain blob (`accuracy_engine.py --spi`) | 5 / 6 / 8 bytes: 675,167 / 761,078 / 977,861 distinct frames, **0 differ** from Python (forest and final) |
| Every dataset frame over SPI, stamped and scrambled blob (`--spi --enc`): the FPGA unscrambles it itself | 5 / 6 / 8 bytes: same frames, **0 differ** |

Quartus Prime Lite 25.1, whole board design, built with a throwaway key (deleted
afterwards):

| | Before Stage 5 | After Stage 5 |
|---|---|---|
| Logic elements | 29,869 (60 %) | 33,253 (67 %) |
| Registers | 7,607 | 9,924 |
| M9K blocks | 106 of 182 | 146 of 182 (80 %): 40 are the AES S-boxes |
| Fmax, slow 85 C (needed: 50 MHz) | 56.96 MHz | 56.18 MHz |
| Builds without `keys/device_key.vh` | | **No**: stops at "can't open device_key.vh", as intended |

- The glitch hardening survived synthesis: `tag_a`, `tag_b` (128 bits each),
  `mac_ok1`, `mac_ok2`, `mac_pass` (32) and both lockout copies are all listed under
  "Registers Protected by Synthesis", none removed. `update_core`'s state register is
  not re-encoded as a state machine, so its `default` branch stays as written.
- `aes_sbox.v` is an initialised array, not a 256-way `case`: Quartus makes the same
  M9K ROMs from both, and Icarus simulates the array about 3 times faster. (A first
  compile with the `case` form gave 33,275 LEs and 57.55 MHz.)
- **The keys appear in the synthesis report**, in binary, in the parameter tables of
  `updatable_top.map.rpt`. Treat `quartus/` outputs as secret.

## Not run: needs the board

| Test | Status |
|---|---|
| UFM behaviour on real silicon (erase, write, read back through the IP) | **Not run** |
| STM32 pushing a model over real wires | **Not run** |
| Four attacks on the ESP32 bench with the SPI-loaded model | **Not run** |
| Unplug the STM32, power cycle, check the model stays | **Not run** |
| The stamp on the board: a wrong key three times, lockout, power cycle (SPI_WIRING test 7) | **Not run** |
| STM32 fw 17 on the four attacks (compare lat, abort, dlate with fw 16) | **Not run** |

Steps are in `docs/SPI_WIRING.md`. The flash model matches the Quartus IP's ports,
but only the board can show how the real flash behaves.

## Cross-check against Intel's flash model

`tb/intel_flash_tb.v` runs the flash controller (`rtl/nv/nv_ufm_avmm.v`) against
Intel's own encrypted simulation model of the On-Chip Flash, in Questa: page erase,
word write, read back, both UFM sectors, and a page erase that must leave another
page alone. **All pass.** This checks the controller's register sequence against
Intel's model. The power-cut and slot tests above still use `tb/ufm_model.v`, which
has to be mine: Intel's model does not simulate a power failure. Re-run after the
timeout fix (2026-11-12): **all pass**.

## Reproducing

`run_sim.bat` runs all 12 steps. Before Stage 5 it ran end to end from an empty
`build/` folder in about an hour. For the Stage 5 numbers above, its steps were run
one by one (the same commands, from a bash script), not the .bat itself; the AES
circuits make the simulations several times slower. `normal_run_data.txt` is
extracted from the dataset zip automatically.

SPI was tested at 2.625 MHz, the speed the STM32 tool uses. The limit is about 4 MHz.
