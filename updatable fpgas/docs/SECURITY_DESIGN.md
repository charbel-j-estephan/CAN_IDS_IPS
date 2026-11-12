# Security design

Sections 1, 2, 4 and 5 are built and simulated (Stage 5, `rtl/security/`).
Section 3 is manual steps on the board, written down only. Nothing here has run
on the board yet.

## Where things stand

The board now checks a **stamp** (AES-CMAC, the MAC field) on every rulebook it
takes over SPI and on every rulebook it boots from flash. A CRC only catches
mistakes, since anyone can compute one; the stamp needs the board's key. The only
rulebook allowed without a stamp is the boot ROM's own (wildcard device ID,
not scrambled), and that exception is checked twice.

Also enforced: maintenance switch (sampled twice), 3 strike lockout (now real:
three wrong stamps lock the update port until a power cycle), anti rollback floor
(checked twice), device ID, protocol ID, range checks, self test, two-slot flash
with journal, and optional **scrambling** of the self tests, key table and nodes.

| Piece | File | Checked against |
|---|---|---|
| AES-128, one round per 2 clocks, S-boxes as M9K ROMs | `rtl/security/aes128_enc.v`, `aes_sbox.v` | FIPS-197 C.1 and 1,000 random blocks (`tb/aes_tb.v`) |
| Seal checker, CMAC over the byte stream | `rtl/security/cmac_stream.v` | RFC 4493 (16, 40, 64 bytes), every real blob length, random lengths (`tb/cmac_tb.v`) |
| Keystream for scrambling, AES-CTR | `rtl/security/ctr_stream.v` | blocks 0-2 for two keys and IVs (`tb/ctr_tb.v`) |
| Wiring, compares, gates, lockout | `rtl/update/update_core.v`, `blob_walker.v`, `blob_reader.v` | `tb/update_tb.v` |
| PC side | `python/blob_crypto.py`, `sign_blob.py`, `make_device_key.py` | `cryptography` library |

Not covered: the 0 byte CMAC case of RFC 4493 (a byte stream of length 0 never
ends a block; a blob is never shorter than 708 bytes), side channel attacks
(power analysis) on the AES circuit, and anything done with a JTAG cable (section 3).

## 1. AES-CMAC inside the FPGA (built)

- `cmac_stream.v` sits on the walker's input, so one circuit sees the bytes from
  SPI, flash and ROM alike. It covers every byte before the MAC, the CRC included.
- It holds a byte back (`ready`) while its AES is busy, instead of assuming it is
  fast enough: at boot the flash feeds bytes about every 2 clocks.
- The walker can finish before the last AES block does; `S_LOAD_CHK` and
  `S_C_CHECK` wait for the tag (at most 255 clocks).
- Cost: about 21 clocks per 16 bytes. A 5 KB blob adds well under a millisecond.

## 2. Keys (built)

- **Signing happens on your PC, never on the STM32.** `python/sign_blob.py`. The
  STM32 holds no key: dumping it gives nothing that can sign a model.
- **Per device keys**, from one master key in `keys/` (ignored by git):
  stamp key = AES-CMAC(master, device ID), scrambling key = AES-CMAC(master,
  `ENC` + device ID). Two keys, never one for both.
- `python/make_device_key.py --key keys/master.key --device-id N` writes
  `keys/device_key.vh` (board number and both keys together). `updatable_top.v`
  includes it; Quartus stops if it is missing, so no board is built with a test
  key. The simulations use `tb/test_key.vh`, from a key printed in
  `blob_crypto.py` on purpose.
- The key is a constant in the design, so **one program file per board**, and it
  appears in Quartus' reports: do not share the `quartus/` output folder.

## 3. Where the device key can live on a MAX 10 (manual steps, not done)

**Warning before any of these.** They are done in the Quartus Programmer on the
board and are hard to undo. JTAG secure mode can stop you reprogramming the board
over the USB-Blaster. Do them only on a finished board, never on your one
development board, and keep the `.ekp` key file somewhere safe.

| Place | Readable? | Verdict |
|---|---|---|
| UFM | Readable through the design and, as far as I could confirm, possibly over JTAG | **No** |
| `.mif` / block RAM init | Inside the bitstream, readable unless encrypted | **No** |
| Logic in the bitstream, with bitstream encryption | Protected by the options below | **Yes, this one** |

So the key is a constant in the RTL, which means **one bitstream per board**.

### What the MAX 10 actually gives you

From Intel's MAX 10 configuration guide:

- **AES-128 bitstream encryption.** The key is stored non-volatile in the internal
  flash and cannot be read through any interface. A full chip erase clears it.
- **It does not cover everything.** The header and I/O configuration data are not
  encrypted; decryption covers the core data and the postamble.
- **JTAG secure mode** limits JTAG instructions. With an encrypted POF, the device
  accepts only files encrypted with the same key and blocks configuration through JTAG.
- **Verify protect** disables read-back of the configuration flash (CFM).
- **No bitstream authentication.** Intel's own notice says the MAX 10 has no internal
  mechanism to authenticate bitstreams and recommends system level controls. Encrypted
  is not the same as authenticated.
- **Region immutability** exists only on DD parts. Yours is `10M50DA`, so no.

What it does **not** give you, plainly:

- Someone with JTAG access can erase the whole device (which clears the key) and load
  their own design, unless JTAG secure mode stops them, and that protection is itself
  stored in the same part.
- I could not confirm from Intel's documents whether UFM can still be read over JTAG
  with verify protect on (verify protect is described for the CFM). **Test this on your
  board before relying on it.** If UFM is readable, blocking READ over SPI does not hide
  the thresholds; only payload encryption does.
- Nothing I read claims resistance to side channel attacks (power analysis) on the
  decryption of the bitstream, or on a soft AES core. Assume none.
- The Unique Chip ID IP (`altchip_id` is in your Quartus install) gives a per-chip
  number. It is public: fine as a device ID, never as a secret.

Build flow for production: generate the encrypted POF with your key (`.ekp`, kept
confidential), enable JTAG secure and verify protect, set `READ_MODEL_EN = 0`.

## 4. Glitch hardening (built, except as noted)

The commit and boot decisions are a target: one flipped bit at the right moment
would accept an unsigned model. What `update_core.v` does:

| Risk | What is built | Simulated |
|---|---|---|
| The stamp compare | Two tag registers (`tag_a`, `tag_b`), compared at two different clocks (`S_C_CHECK` and `S_C_MAC`; at boot `S_LOAD_CHK` and `S_LOAD_MAC2`), each a 128 bit XOR then OR, the same time whatever the data | forcing the first compare to "yes" still gives `E_MAC` |
| A single "OK" bit | `mac_pass` is set to the 32 bit code `C3A55A3C` only when both compares agree. `S_C_ERASE` and the boot `S_CFG` refuse to run without it, also if a glitch jumps straight into them. Cleared on every new upload, failure and activation | |
| The ROM exception (the one blob allowed without a stamp) | ROM source, wildcard device ID and flags 0, checked in `S_LOAD_CHK` and again in `S_LOAD_PASS` | the ROM still boots on empty flash |
| Rollback | the version against the floor in `S_C_CHECK` and again in `S_C_MAC` | |
| Lockout counter | two copies, the second inverted; any disagreement counts as locked | forcing a disagreement locks the port |
| Maintenance switch | sampled every clock and every 128 clocks; both must be up (`updatable_top.v`) | board only |
| Merging by Quartus | `(* preserve *)` on the duplicated registers; check the fitter report | see `docs/RESULTS.md` |
| State register | the `default` branch sends an illegal state to `S_HALT` (passive). Not one-hot or Hamming coded | |
| Journal marker write | unchanged: the record's check word already protects it, and it is written last | |

Left out on purpose: doubling the one bit bank tag in the engine pipeline (it
costs a register on every level of every tree, and the self test already catches
a bank that does not classify).

## 5. Optional confidentiality (built: `sign_blob.py --encrypt`)

Encrypting the payload keeps the trees secret on the wire and in flash, which is what
really stops someone learning the thresholds from a captured update or a flash dump.

- Mode: AES-128-CTR, IV from the blob header (bytes 40 to 51) plus a 32 bit block
  counter; the IV must never repeat under the same key.
- Encrypt then MAC: the MAC covers the ciphertext, so the FPGA rejects a bad blob
  before decrypting anything.
- The header stays in the clear (it holds the version, device and lengths the FPGA
  needs before it can decrypt). Encrypted: self test vectors, key table, nodes.
- The flag is `Flags` bit 0. The boot ROM blob may never have it.
- Unscrambled in `blob_walker`'s byte path, between the source and the parser; the
  engine does not change. The RAMs hold the plain model, so `blob_reader`'s output is
  scrambled again on the way to flash: the CRC check of the RAM, the flash write and
  the flash read back all see the scrambled bytes, and **flash never holds the plain
  rulebook**. A test compares the flash with the plain blob.
- `sign_blob.py --encrypt` picks a fresh random IV every time and offers no way to
  choose one.
- A development build (`READ_MODEL_EN = 1`) can still read the plain model out of the
  RAMs over SPI. Production builds must use `READ_MODEL_EN = 0`.
