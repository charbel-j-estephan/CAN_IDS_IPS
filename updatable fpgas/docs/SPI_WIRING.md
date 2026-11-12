# Wiring the STM32 update tool, and the bench steps

The STM32 is only an update tool. Plug it in, push a model, unplug it.

## Wiring

Both boards are 3.3 V, so the wires go straight across. **Connect GND first and
remove it last.**

| Signal | DE10-Lite header | FPGA | STM32 black F407VE |
|---|---|---|---|
| SCLK | pin 5, GPIO[4] | PIN_V8 | PB13 |
| MOSI | pin 6, GPIO[5] | PIN_W8 | PB15 |
| MISO | pin 7, GPIO[6] | PIN_V7 | PB14 |
| CS_n | pin 8, GPIO[7] | PIN_W7 | PB12 |
| GND | pin 12 | | GND |

The STM32 is powered from its own USB, and is not powered from the FPGA. PB12 to
PB15 also go to the board's TFT touch header; leave no TFT fitted.

Switches: **SW[1] up = maintenance mode** (the port accepts updates), LEDR[8] shows it.
SW[0] stays prevention.

## One-time setup

1. **Make your master key and this board's key file** (once per board; keep the
   master key backed up off this PC: lose it and you can no longer update the board):

```bash
py -3.13 python/sign_blob.py --new-key keys/master.key
```

```bash
py -3.13 python/make_device_key.py --key keys/master.key --device-id 1
```

   The second writes `keys/device_key.vh` (board number and its two keys).
   `keys/` is ignored by git. Quartus stops if the file is missing.
2. Compile `quartus/updatable_top.qpf`. The key ends up in the program file and in
   Quartus' reports: do not share the `quartus/` output folder.
3. **Program the `.pof`, CFM only.** In the Quartus Programmer, add
   `output_files/updatable_top.pof`, tick **CFM0** (and any CFM entries shown) under
   Program/Configure and leave **UFM unticked**. Programming the UFM erases the
   stored models. A `.sof` also works for testing, but it is lost at power off.
4. Build and flash the update tool (`stm32_updater`), the same BOOT0 steps as fw 16:

```bash
pio run -e black_f407ve_dfu -t upload
```

First power on with an empty UFM: the board boots the 5-byte model from its ROM
(LEDR[5:4] = 3).

### Warnings after the security update (plain words)

- **Rulebooks saved before it stop working.** They have no valid stamp for this
  board, so at power on the board falls back to the ROM model (LEDR[5:4] = 3)
  until you push a stamped one.
- **Use a higher version than anything ever pushed to this board.** It remembers
  the highest version it accepted (READ register `0028`), including old test
  pushes. A lower one is refused with `err=7`.
- **The lockout is real now.** Three pushes with a wrong stamp (wrong key or wrong
  device ID in `make_device_key.py`) give `err=12` three times, then `err=2`
  until you unplug the board's power. KEY[0] does not unlock it.

## Pushing a model

```bash
py -3.13 python/export_blob.py --model <rf5_d9_p5_time.joblib> --out build/blobs/p5
```

```bash
py -3.13 python/sign_blob.py build/blobs/p5/model.bin --device-id 1 --version 2 --key keys/master.key --encrypt --out build/signed/p5_v2.bin
```

```bash
py -3.13 python/push_model.py COM5 build/signed/p5_v2.bin
```

`--encrypt` scrambles the self tests, key table and nodes; leave it out for a
plain (still stamped) blob. SW[1] must be up for the push; put it down afterwards.
`push_model.py COM5` alone prints status, version and counters. It needs pyserial
for your Python (`py -3.13 -m pip install pyserial`, or use the bench's venv).

## Bench test 7: the stamp on real hardware

1. Push a stamped rulebook as above: `OK`, LEDR[5:4] shows slot 1 or 2.
2. Sign the same blob with a different key file (`--new-key` into another file)
   and push it three times: `err=12` each time, then STATUS shows
   `locked=1`. Push the good one: `err=2`.
3. Unplug the board's power, plug it back: the good rulebook is still active and
   the port is open again.

## Bench test 5: the four attacks with the SPI-loaded model

Not run by me: this needs the hardware.

1. Program and push the 5-byte model as above. Check `VERSION model=2DDDA531 version=2`.
2. SW[0] up (prevention on), SW[1] down, STM32 unplugged or left idle.
3. From `Desktop\esp32 setup\can_replay` run the same four tests as for the
   hardcoded design (`run_check_esp32.bat`, one per attack file, with the same
   arguments you used on 11-03).
4. Compare with `docs/HANDOVER.md`, 11-03 FPGA IPS 5 bytes. The result to match:
   DoS 1,861/1,861 stopped, Fuzzy 709/709, Gear 922/922, RPM 1,085/1,085, **0 normal
   frames lost** in all four, attacker bus off in every run.

## Bench test 6: unplug and power cycle

Not run by me: this needs the hardware.

1. Push a new model with a higher version number (for example the 8-byte model,
   `--version 3`). LEDR[5:4] shows slot 1 or 2.
2. Unplug the STM32 completely (all five wires).
3. Power cycle the DE10-Lite (USB out, wait, USB in).
4. Plug the STM32 back in with SW[1] down and run `push_model.py COM5`: GET_VERSION
   still works without maintenance mode. It must show the new model and version.
5. Unplug the STM32 again and run the attack bench: it must still protect the bus.

## If something looks wrong

| Sign | Cause |
|---|---|
| `STATUS` is `FFFFFFFF` or the top byte is not `C1` | wiring, or no GND |
| `err=1` | SW[1] is down |
| `err=2` | locked out: 3 signature failures; power cycle the board |
| `err=12` | stamp wrong: signed with another key, or `keys/device_key.vh` made for another device ID |
| `err=6` | blob signed for another device ID |
| `err=7` | version lower than the one already accepted |
| LEDR[5:4] = 3 after a power cycle | the flash slots failed their checks and the ROM model is running |
