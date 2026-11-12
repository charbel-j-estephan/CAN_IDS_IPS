# SPI update port: commands and register map

SPI mode 0, MSB first, one command per chip select, STM32 as master. SCLK 2.625 MHz:
that is the speed tested in simulation and the one the update tool uses. The limit is
about 4 MHz. The FPGA samples SCLK with its 50 MHz clock, and right after blob offset
175 it spends 96 clocks checking the node counts, so a byte must take at least 100 clocks.
Multi byte SPI fields are big endian. Registers are 32 bits.

| Command | Byte | Then | Maintenance needed |
|---|---|---|---|
| WRITE_SHADOW | `01` | address (2), length (2), data | Yes |
| READ | `02` | address (2), dummy (1), data (4 out) | Yes |
| COMMIT | `03` | expected CRC (4), then poll STATUS | Yes |
| STATUS | `04` | dummy (1), status (4 out) | No |
| GET_VERSION | `05` | dummy (1), model ID, version, info (12 out) | No |
| READ_COUNTERS | `06` | dummy (1), seen, blocked, errors, sig fails (16 out) | Yes |

Without maintenance mode (SW[1] down) every command except STATUS and GET_VERSION
is refused and counted. After 3 signature failures WRITE_SHADOW and COMMIT are
refused (`E_LOCKED`) until a power cycle; KEY[0] does not clear it. The read
commands keep working so a locked board can be examined.

WRITE_SHADOW must be in order: address 0 starts a new upload, any other address
must equal the bytes received so far. A chunk may be up to 65,535 bytes; the tool
uses 256. Address is the blob offset.

COMMIT is asynchronous: send it, then poll STATUS until bits 8 and 15 are clear,
then read LAST_ERROR (0 = success). A COMMIT takes up to about 1.5 s on the board.

## Registers (READ address)

| Address | Name | Meaning |
|---|---|---|
| `0000` | STATUS | as below |
| `0004` | MODEL_ID | active model ID (0 if none) |
| `0008` | VERSION | active model version |
| `000C` | SOURCE | bits 1:0: 0 none, 1 slot A, 2 slot B, 3 boot ROM |
| `0010` | FRAMES_SEEN | verdicts given since reset |
| `0014` | FRAMES_BLOCKED | error flags sent since reset |
| `0018` | UPDATE_ERRORS | refused commands and failed COMMITs since reset |
| `001C` | SIG_FAILS | MAC failures since reset |
| `0020` | LAST_ERROR | code below |
| `0024` | SHADOW_OFFSET | blob bytes received so far |
| `0028` | ROLLBACK_FLOOR | highest version accepted, from flash |
| `002C` | DEVICE_ID | this board's ID (build time) |
| `0030` | PARSE_ERRORS | blob_walker sticky bits, below |

### STATUS

| Bits | Meaning |
|---|---|
| 7:0 | LAST_ERROR |
| 8 | Busy (boot or COMMIT running). 0 with source 0 after boot: no usable model anywhere (not even the ROM); classification is off and the port takes a new model |
| 9 | Locked out |
| 10 | Maintenance switch on |
| 11 | Active bank |
| 13:12 | Source of the active model (as SOURCE) |
| 14 | Shadow bank holds a complete blob |
| 15 | Swap waiting for a frame boundary |
| 21:16 | State machine (for debugging) |
| 23:22 | 0 |
| 31:24 | `C1`, so a dead bus does not read as a good status |

### Error codes

| Code | Name | Meaning |
|---|---|---|
| 0 | OK | |
| 1 | NOT_MAINT | Maintenance switch off |
| 2 | LOCKED | Locked out after 3 signature failures |
| 3 | TRUNCATED | Blob not fully received |
| 4 | HEADER | Magic, format, ranges or feature map wrong |
| 5 | PROTOCOL | Wrong protocol ID |
| 6 | DEVICE | Wrong device ID |
| 7 | VERSION | Lower than the highest accepted version |
| 8 | RANGE | Feature index, child pointer, leaf vote or node counts out of range |
| 9 | LENGTH | Length does not match the counts, or bytes after the end |
| 10 | CRC | CRC of the blob or of the shadow RAM wrong |
| 11 | SELFTEST | A self test vote did not match |
| 12 | MAC | Stamp (AES-CMAC) wrong or missing. Counts toward the lockout. Also: the stamp never finished (no strike) |
| 13 | ERASE | Flash erase failed |
| 14 | WRITE | Flash write failed |
| 15 | VERIFY | Flash read back differs |
| 16 | BUSY | Another operation is running |
| 17 | SEQUENCE | WRITE_SHADOW out of order, or data arrived too fast |

### PARSE_ERRORS bits

0 header, 1 node counts, 2 feature index, 3 child pointer, 4 leaf vote, 5 length,
6 bytes after the end.

## Reading the model (development builds only)

Build with `READ_MODEL_EN = 1` to read the **shadow** bank. A production build
(`READ_MODEL_EN = 0`) answers all of these with `DEAD0BAD`, so thresholds never
leave the chip over SPI.

| Address | Width | Meaning |
|---|---|---|
| `1000` to `11BF` | 8 | header, feature map, counts, self tests (blob offset 0 to 447) |
| `2000` to `20FF` | 8 | key table bytes |
| `4000` + (tree << 11) + (level << 7) + index | 32 | node bits 31:0 |
| `8000` + (tree << 11) + (level << 7) + index | 4 | node bits 35:32 |

Any READ whose answer is not ready by the end of the dummy byte, and any node
address with level 12 to 15 (there is none), also reads `DEAD0BAD`, never an
older value. The RAMs hold the plain model even for a scrambled blob: a
development build reads it in the clear.

## Flash layout (UFM, 32 bit words)

| Words | Sector | Use |
|---|---|---|
| 0 to 6143 | UFM1 pages 0-2 | Slot A |
| 6144 to 8191 | UFM1 page 3 | Journal page A |
| 8192 to 14335 | UFM0 pages 0-2 | Slot B |
| 14336 to 16383 | UFM0 page 3 | Journal page B |

A journal record is 4 words, appended: `4C4E524A`, {sequence (24), 0 (7), slot (1)},
version, NOT of the XOR of those three. The newest record whose check passes picks
the active slot; the highest version in any valid record is the rollback floor.
