# Updatable CAN IDS/IPS

The same CAN IDS/IPS as `quartus_board/`, except the random forest is data in
memory: frozen RTL, a model you load over SPI, kept in on-chip flash, and portable
to an ASIC. Nothing outside this folder was changed; the hardcoded build stays
in `quartus_board/` for comparison.

| Folder | What it is |
|---|---|
| `rtl/engine/` | The frozen, protocol-free tree engine. No CAN anywhere |
| `rtl/frontend_can/` | The CAN specific part: decoder, feature extractor, IPS. Replace this folder for another protocol |
| `rtl/update/` | SPI, commands, blob parser and reader, commit and boot state machine |
| `rtl/nv/` | Flash back end and boot ROM (the FPGA specific parts) |
| `rtl/top/` | `updatable_core.v` (no vendor parts), `updatable_top.v` (DE10-Lite) |
| `python/` | Exporter, signer, push tool, tests |
| `stm32_updater/` | STM32 update tool firmware (separate from fw 16) |
| `quartus/` | `updatable_top` (full board), `engine_only` (classifier alone) |
| `tb/` | Icarus testbenches and the flash model |
| `docs/` | `BLOB_FORMAT`, `REGISTER_MAP`, `SPI_WIRING`, `ASIC_BOOT`, `SECURITY_DESIGN`, `RESULTS` |

Start with `docs/SPI_WIRING.md` for the board, `docs/RESULTS.md` for what was
measured and what was not.

## Run the simulations

From this folder, after training (the models come from the repo's `models/`):

```bash
run_sim.bat
```

`run_sim.bat` exports the blobs, makes the test blobs and runs every simulation.
