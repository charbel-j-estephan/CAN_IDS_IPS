# ASIC plan: the same boot loader without UFM

An ASIC has no bitstream and no on-chip flash, so everything that can change is
already data. Only two files are FPGA specific, and both sit behind small
interfaces:

| FPGA | ASIC replacement | Interface |
|---|---|---|
| `rtl/nv/nv_ufm_avmm.v` + Altera On-Chip Flash IP | a controller for an external SPI NOR flash (for example 1 Mbit, 4 KB erase blocks) | op (read, write, erase page), addr, wdata, start, done, ok, rdata |
| `rtl/nv/boot_rom.v` + `.mif` | a real ROM macro, mask-programmed with the default blob | addr in, byte out, one clock |
| `rtl/engine/sdp_ram.v` (M9K) | the foundry SRAM macro | one write port, one read port |

Nothing else changes: `rf_engine`, `rf_classifier`, `blob_walker`, `blob_reader`,
`update_core`, `cmd_decoder` and the front end are plain synthesizable Verilog
with no vendor parts.

## How boot works there

1. A power-on reset pin replaces the FPGA's start-up counter (`por`), the only
   thing that clears the update lockout.
2. `update_core` scans the journal, loads the newest valid slot from flash, checks
   it, runs its self test and activates it, exactly as on the FPGA. Same blob,
   same parser.
3. If both slots fail it loads the ROM blob through the same path.

The flash layout (two slots, two journal pages) maps directly: each slot is 24 KB
and each journal page is one erase unit. A 1 Mbit part holds both with room left;
a larger one allows bigger models.

## Things to plan differently

- **External flash is on the board, so it can be read and rewritten.** The MAC
  check must run at every boot, not only at COMMIT, and any model that must stay
  secret needs payload encryption. On the FPGA the UFM is at least inside the package.
- **Keys** cannot sit in a bitstream. Use one-time programmable fuses or a secure
  element, written once per chip at manufacturing.
- **Default model in ROM** cannot be updated after tape-out, so it should be a
  conservative model; a field update replaces it through the normal path.
- **Device ID** should come from a fuse block rather than a build parameter.
