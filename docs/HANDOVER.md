# Handover: CAN bench status

Branch `claude/esp32-can-bus-wiring-372c60`. The repo is the master copy;
`Desktop\esp32 setup\can_replay` is the copy the runs are done from
(`python python/sync_bench.py` updates it).

## Working (tested on hardware)

- 2-ESP32 bench (`replay_sender` + `usb_receiver`), see `docs/ESP32_WIRING.md`.
- Runs: DoS 20,000, Fuzzy 20,000, gear 100,000 frames: 100% delivered,
  timing within a median of 6 to 7 microseconds of the original capture
  (95% within 13 to 22 microseconds), each frame ending at its original time
  (exact length on the wire from `python/can_bits.py`).
- STM32 upload over USB DFU: BOOT0 jumper (BT0 to 3V3), RST, Upload, remove
  jumper, RST. The Zadig driver for "STM32 BOOTLOADER" must be libusbK.

## Next (approved plan)

1. Attacker on its own ESP32 (third board on USB): normal frames from A,
   attack frames from C, both clocks started from one marker frame on the bus.
2. STM32 alone IDS (report Fig. 3): bit-level reception in software on PB8,
   then the emlearn random forest per frame.

## Known limits

- The dataset's own timestamps are noisy (some gaps are physically impossible),
  so timing cannot be judged finer than about 10 microseconds.
- The current trained model uses ID, DLC and data only, with a random split;
  it must be retrained with a time split and the report's features before
  any accuracy figure goes into the report.
- Icarus Verilog is installed in `C:\iverilog\bin`, but the PATH says
  `C:\iverlog\bin` (missing i).
