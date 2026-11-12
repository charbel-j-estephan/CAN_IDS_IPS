# The model blob

One file holds a whole model. `python/export_blob.py` makes it from a trained
sklearn forest; `python/blob_format.py` is the reference for the format.
Multi byte integers are little endian.

| Offset | Size | Content |
|---|---|---|
| 0 | 64 | Header |
| 64 | 16 | Feature map |
| 80 | 96 | Node counts: one byte for each (tree, level), 8 trees x 12 levels |
| 176 | 272 | Self test vectors: 8 x 34 bytes |
| 448 | 256 | Key table bitmap (known CAN IDs): key `k` is byte `k >> 3`, bit `k & 7` |
| 704 | 5 x S | The S split nodes, in order tree, level, index |
| 704 + 5S | 4 | CRC-32 of every byte before it (IEEE, same as Python `zlib.crc32`), scrambled bytes as they are |
| 708 + 5S | 16 | MAC (the stamp): AES-CMAC of every byte before it, the CRC included |

Total length = 724 + 5 x S bytes. A blob must fit one flash slot, 24,576 bytes,
which is at most 4,770 splits. The FPGA rejects anything larger.

## Header

| Offset | Size | Field | Notes |
|---|---|---|---|
| 0 | 4 | Magic | `CIFM`, `0x4D464943` |
| 4 | 2 | Format version | 1 |
| 6 | 2 | Flags | bit 0 = scrambled (bytes 176 to the CRC); bits 15:1 must be 0. Never set in the boot ROM blob |
| 8 | 4 | Total length | bytes, whole blob |
| 12 | 4 | Model ID | hash from the model name, e.g. `2ddda531` |
| 16 | 4 | Version | monotonic counter (anti rollback) |
| 20 | 4 | Device ID | `FFFFFFFF` = any device, accepted **only** from the boot ROM |
| 24 | 2 | Protocol ID | 1 = CAN classic, 11 bit IDs |
| 26 | 1 | Tree count | 1 to 8 |
| 27 | 1 | Depth | levels used, 1 to 12 |
| 28 | 1 | Feature count | 1 to 16 |
| 29 | 1 | Class count | 1 to 8 |
| 30 | 1 | Front end config | CAN: data bytes the model reads, 0 to 8 |
| 31 | 1 | Rule enable | bit 0: key table rule on |
| 32 | 1 | Rule from class | Normal = 0 |
| 33 | 1 | Rule to class | Fuzzy = 2 |
| 34 | 1 | Self test count | 1 to 8 |
| 35 | 1 | Reserved | 0 |
| 36 | 4 | Key ID | which signing key (written by `sign_blob.py`, not checked by the FPGA) |
| 40 | 12 | IV | scrambling: 12 random bytes, new for every scrambled blob |
| 52 | 12 | Reserved | 0 |

## Feature map

One byte per feature slot: what the slot holds, or `FF` for unused. The FPGA
compares it with its front end, so a model built for different features cannot
load. CAN front end: 0 = ID, 1 = DLC, 2 to 9 = data bytes 0 to 7, the rest unused.

## Self test vector (34 bytes)

16 x 16 bit feature values, 1 byte expected forest vote, 1 byte reserved. The
FPGA runs each one through the shadow bank before a COMMIT or a boot succeeds.

## Node (5 bytes, 36 bits used)

Packed low bits first: right (8), left (8), threshold (16), feature (4), then
4 bits zero. `left` and `right` are each `{is_leaf, 7 bit index or class}`. A
non-leaf child points into the next level. The rule is sklearn's: go left when
`feature <= threshold`, with the integer threshold `floor(t)`.

## The stamp and the scrambling

`python/blob_crypto.py` is the reference; `python/sign_blob.py` uses it. Keys,
all AES-128, from one master key that stays on the PC:

| Key | Made as | Used for |
|---|---|---|
| stamp key | AES-CMAC(master, device ID as 4 bytes little endian) | the MAC |
| scrambling key | AES-CMAC(master, `ENC` + device ID as 4 bytes little endian) | the keystream |

- **Stamp (MAC):** AES-CMAC (RFC 4493) with the stamp key over every byte before
  the MAC, the 4 CRC bytes included. The FPGA computes it as the blob streams in
  (`rtl/security/cmac_stream.v`) and compares it with this field at every COMMIT
  and at every boot from flash. The boot ROM blob carries no stamp: it is the
  only source allowed to (device ID `FFFFFFFF`, flags 0).
- **Scrambling (AES-CTR):** with Flags bit 0 set, bytes 176 up to the CRC (self
  tests, key table, nodes) are xored with AES(scrambling key, IV ‖ block), the
  block number 32 bits big endian from 0, one block per 16 bytes. The header,
  feature map and node counts stay plain: the FPGA needs them before it can
  unscramble. CRC and MAC are worked out on the scrambled bytes (encrypt then
  MAC), so a bad blob is refused before anything is unscrambled. The FPGA keeps
  the scrambling in flash: it scrambles again on the way out of its RAM.

## What the FPGA checks

While the blob arrives (`blob_walker.v`): header ranges, feature map, the shape
of every tree, every feature index, every child pointer, every leaf vote, the
total length, and at least 5 data bytes read (the IPS allow list needs bytes 0-4).
At COMMIT: all of that, then protocol, device, version, CRC of
what is actually in RAM, self test, MAC (checked twice), version again. See `REGISTER_MAP.md` for the error codes.
