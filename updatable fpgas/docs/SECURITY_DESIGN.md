# Security design (Phase D: design only, nothing here is implemented yet)

## Where things stand today

Phase 1 checks a CRC. **A CRC catches mistakes; it does not stop an attacker**, who
can compute one for any blob. The self test vectors come from the same blob, so
they catch hardware faults, not malice. Until the MAC exists, the only thing
standing between an attacker and the update port is the physical maintenance
switch. The port is on the board, so anyone with access to the board has access to
the switch.

Already in the hardware today, and enforced: maintenance switch, 3 strike lockout
(fires only when a MAC check exists to fail; a test hook forces it in simulation),
anti rollback counter, device ID check, protocol ID check, range checks, self test,
two-slot flash with journal. Blob fields for MAC, key ID, IV and the encryption flag
all exist, so the MAC goes in without changing the format or the register map.

## 1. AES-CMAC inside the FPGA

Your AES-128 engine is **not in this repository or on the Desktop** (searched for
`*aes*` Verilog and VHDL files). Point me to it when this phase starts. The design
below assumes a block engine with key, block in, block out, start and done.

- A new module `mac_verify.v` replaces the `mac_ok` stub input of `update_core`. It
  needs no new ports on anything else.
- It runs in `S_C_MAC` for COMMIT and in `S_LOAD_CHK` for boot. Reading the blob back
  through `blob_reader` (RAM) or the flash source gives it exactly the bytes signed.
- CMAC per RFC 4493: subkeys K1, K2 from AES(K, 0); 16 byte blocks; the last block
  padded or XORed with the subkey.
- Cost: about 4,200 to 5,100 bytes, 260 to 320 blocks. At about 11 clocks per AES
  round-based block that is well under 100 us. Boot and COMMIT time do not change
  noticeably.

## 2. Keys

- **Signing happens on your PC, never on the STM32.** `python/sign_blob.py` does this
  now. The STM32 holds no key: dumping it gives nothing that can sign a model.
- **Per device keys.** Device key = AES-CMAC(master key, device ID), computed on the
  PC. Each board stores only its own device key, so one extracted key does not unlock
  other boards. The master key stays on the PC, in `keys/` (ignored by git).
- Use separate derived keys for the MAC and for payload encryption (different
  derivation labels); never one key for both.

## 3. Where the device key can live on a MAX 10

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

## 4. Glitch hardening

The commit path is a target: one flipped bit at the right moment would accept an
unsigned model.

- **Constant time compare.** OR together the XOR of every MAC byte; never stop at the
  first difference.
- **Compare twice**, independently, in two separate registers and at two different
  times.
- **Multi bit pass value.** The result register holds a 32 bit magic constant set only
  when both compares agree, and `S_C_ERASE` is entered only if it equals the magic.
  Never a single flag bit.
- **Other risks on the commit path, for the same treatment:**
  - the state machine: use a one-hot or Hamming distance encoded state register for the
    commit states, and treat any illegal value as a failure (`default` already does this);
  - the version compare (a glitch could accept a rollback): compare twice;
  - the lockout counter: use a redundant encoding, so a glitch cannot reset strikes;
  - the journal marker write: the record's check word already protects it, keep it last;
  - clock glitching during the bank swap: the bank tag is one bit in the pipeline, so
    consider duplicating it;
  - the maintenance switch input: sample it twice, a few clocks apart.

## 5. Optional confidentiality

Encrypting the payload keeps the trees secret on the wire and in flash, which is what
really stops someone learning the thresholds from a captured update or a flash dump.

- Mode: AES-128-CTR, IV from the blob header (bytes 40 to 51) plus a 32 bit block
  counter; the IV must never repeat under the same key.
- Encrypt then MAC: the MAC covers the ciphertext, so the FPGA rejects a bad blob
  before decrypting anything.
- The header stays in the clear (it holds the version, device and lengths the FPGA
  needs before it can decrypt). Encrypted: self test vectors, key table, nodes.
- The flag is `Flags` bit 0, already rejected today, so an encrypted blob cannot be
  misread as a plain one by this build.
- Decrypt in `blob_walker`'s byte path, between the source and the parser. The engine
  does not change.
