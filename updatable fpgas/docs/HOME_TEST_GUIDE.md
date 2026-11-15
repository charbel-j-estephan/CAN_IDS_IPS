# Home test guide: the updatable FPGA, step by step

You can test on the DE10-Lite at home, without the CAN bench. Do the parts in
order. Each step says what you should see. Tick the box when it matches; if it
doesn't, stop and check the table at the end before going on.

| Part | What you need | Time | What it proves |
|---|---|---|---|
| 1. PC setup | this PC | 45 min (25 of it Quartus) | your key, signed rulebooks, a build |
| 2. Board alone | DE10-Lite, USB cable, 1 jumper wire | 15 min | it boots the ROM model, the switches and LEDs work |
| 3. Updates | + the black STM32 board and 5 jumper wires | 60 min | stamped and scrambled updates, flash slots, rollback, lockout, boot from flash |
| 4. Later, on the bench | the ESP32 CAN bench | | the IPS on a real bus (not at home) |

**Important:**
- Run every command below from
  `C:\Users\Charbel\Desktop\CAN_IDS_IPS\updatable fpgas`.
- **Part 3 replaces the STM32's program.** The STM32 IDS firmware (fw 16) gets
  overwritten by the update tool. Flash fw 17 back before the next bench day
  (`mcu/stm32_ids`, the same DFU steps).

---

## Part 1: PC setup (once)

Open PowerShell in the folder:

```bash
cd "C:\Users\Charbel\Desktop\CAN_IDS_IPS\updatable fpgas"
```

**1.1 Install the serial library** for `push_model.py` (once):

```bash
py -3.13 -m pip install pyserial
```

**1.2 Make your master key.** This is the real secret. Back up
`C:\Users\Charbel\Desktop\master.key` to a USB stick or a password manager. If you lose it, you can
no longer update this board without rebuilding it with a new key.

```bash
py -3.13 python/sign_blob.py --new-key C:\Users\Charbel\Desktop\master.key
```

**1.3 Make this board's key file** (board number 1):

```bash
py -3.13 python/make_device_key.py --key C:\Users\Charbel\Desktop\master.key --device-id 1
```

- [ ] It prints `Wrote ...keys\device_key.vh for device 1`.

**1.4 Make the rulebooks to push.** The plain model files are already in
`build\blobs\p5` and `build\blobs\p8`. If they are missing, make them first:

```bash
py -3.13 python/export_blob.py --model "C:\Users\Charbel\Desktop\CAN_IDS_IPS\models\rf5_d9_p5_time.joblib" --out build/blobs/p5
```

```bash
py -3.13 python/export_blob.py --model "C:\Users\Charbel\Desktop\CAN_IDS_IPS\models\rf5_d9_p8_time.joblib" --out build/blobs/p8
```

Then sign them. Four files: two good ones, and one made with a wrong key.

```bash
py -3.13 python/sign_blob.py build/blobs/p5/model.bin --device-id 1 --version 2 --key C:\Users\Charbel\Desktop\master.key --encrypt --out build/signed/p5_v2.bin
```

```bash
py -3.13 python/sign_blob.py build/blobs/p8/model.bin --device-id 1 --version 3 --key C:\Users\Charbel\Desktop\master.key --encrypt --out build/signed/p8_v3.bin
```

```bash
py -3.13 python/sign_blob.py --new-key build/signed/wrong.key
```

```bash
py -3.13 python/sign_blob.py build/blobs/p5/model.bin --device-id 1 --version 4 --key build/signed/wrong.key --encrypt --out build/signed/wrongkey_v4.bin
```

- [ ] Each sign line prints `Signed for device 1, version N, scrambled: model ...`.
  Note the model IDs: 5 bytes is `2ddda531`, 8 bytes is `c0f7e8b5`.

**1.5 Build the FPGA program.** Open `quartus\updatable_top.qpf` in Quartus and
press Start Compilation (about 25 minutes), or from PowerShell:

```bash
cd quartus
```

```bash
C:\altera_lite\25.1std\quartus\bin64\quartus_sh.exe --flow compile updatable_top
```

```bash
cd ..
```

- [ ] It ends with 0 errors.
- [ ] Timing: in the report, Timing Analyzer > Slow 1200mV 85C Model > Fmax is
  **above 50 MHz** (expect about 55 MHz).

The program file now contains your key. **Never share or zip the `quartus\`
folder.** `keys\` and the Quartus outputs are already kept out of git.

---

## Part 2: the board alone

**2.1 The RX pin.** With nothing on the CAN pins, the FPGA's receive pin floats
and reads noise. Unless your transceiver board is plugged in and powered, put
**one jumper wire from header pin 1 (GPIO[0]) to header pin 29 (3.3 V)**. A
1 kΩ to 10 kΩ resistor instead of a bare wire is even safer. **Never pin 11
(5 V).** Leave pin 2 (TX) unconnected.

**2.2 Connect and check the cable.** Plug the DE10-Lite into USB.

```bash
C:\altera_lite\25.1std\quartus\bin64\quartus_pgm.exe -l
```

- [ ] It lists `USB-Blaster [USB-0]`. If it doesn't, see "If something looks
  wrong" at the end.

**2.3 Load the design into the FPGA** (`.sof`: quick and safe, gone at power off):

```bash
C:\altera_lite\25.1std\quartus\bin64\quartus_pgm.exe -m jtag -o "p;quartus/output_files/updatable_top.sof"
```

**2.4 Look at the LEDs.** Both switches down.

| LED | Expected | Meaning |
|---|---|---|
| LEDR5 and LEDR4 | **both on** | source 3: the built-in ROM model is running (the flash is empty) |
| LEDR6 | off | update port not locked |
| LEDR7 | off | no update error since reset |
| LEDR8 | off | maintenance mode off (SW1 down) |
| LEDR9 | off | prevention off (SW0 down) |
| LEDR0 to LEDR3 | off | no CAN traffic, no errors |

- [ ] LEDR4 and LEDR5 on, everything else off.

**2.5 The switches.**
- [ ] SW1 up: LEDR8 on. Down: off.
- [ ] SW0 up: LEDR9 on. Down: off.

**2.6 Reset.** Press KEY0.
- [ ] The LEDs come back exactly as in 2.4 within a second.

The board alone can't show more than this. Part 3 talks to it.

---

## Part 3: updates with the STM32 tool

### 3.1 Put the update tool on the STM32

1. BOOT0 jumper: **BT0 to 3V3** (BT1 stays on GND), press RST. Windows shows a
   DFU device.
2. Upload:

```bash
cd stm32_updater
```

```bash
C:\Users\Charbel\.platformio\penv\Scripts\pio.exe run -e black_f407ve_dfu -t upload
```

```bash
cd ..
```

   A first line `dfuERROR ... firmware is corrupt` is normal (the chip was
   started in upload mode); it must end with `File downloaded successfully`.

3. **BT0 back to GND**, press RST. The STM32 appears as a new COM port. Find
   its number in Device Manager > Ports (below, `COM5`; use yours).

### 3.2 Wire the two boards

**Unplug both from USB first. GND first, and remove it last.**

| Wire | DE10-Lite header pin | STM32 pin |
|---|---|---|
| GND | 12 | GND |
| SCLK | 5 | PB13 |
| MOSI | 6 | PB15 |
| MISO | 7 | PB14 |
| CS | 8 | PB12 |

Both boards are 3.3 V, so the wires go straight across. Each board gets its own
USB cable. No TFT screen on the STM32 (it shares these pins). Keep the RX
jumper from 2.1.

Plug both into USB, then load the FPGA design again (step 2.3: it was lost
when you unplugged).

### 3.3 First contact (SW1 down)

```bash
py -3.13 python/push_model.py COM5
```

- [ ] `STATUS C1......` with `busy=0 locked=0 maint=0 source=3`. `err=0` on
  the first run after loading or KEY0, `err=1` on later runs (next point).
- [ ] `VERSION model=2DDDA531 version=1` (the ROM model).
- [ ] `COUNTERS` all `4294967295` (FFFFFFFF). Correct: the counters are only
  given in maintenance mode, so with SW1 down the FPGA refuses (`err=1`,
  LEDR7 on).

Then **SW1 up** and run it again:

- [ ] `COUNTERS seen=0 blocked=0 errors=0 sig_fails=0`, `maint=1`.

**SW1 down** again and press **KEY0** (LEDR7 off) before 3.4.

If STATUS reads `FFFFFFFF`, or doesn't start with `C1`, check the wiring and
GND before going on.

Then check the link really holds (10,000 reads per line, a few seconds):

```bash
py -3.13 python/spi_test.py COM5 10000
```

- [ ] `ID updater 3...`, and the first two `TEST` lines (`edge=0`) show
  `bad=0`. The third line (faster edges) may lose some: that is the ringing
  on jumper wires the slow edges avoid. Any loss at `edge=0`: reseat and
  shorten the wires, add a second GND (pin 30), and run it again.

### 3.4 The update port is shut without maintenance mode (SW1 still down)

```bash
py -3.13 python/push_model.py COM5 build/signed/p5_v2.bin
```

- [ ] `ERROR upload refused: STATUS ... err=1`. LEDR7 comes on (an update
  error since reset).
- [ ] LEDR4 and LEDR5 still both on: the ROM model is still running.

### 3.5 First real update: 5-byte model, stamped and scrambled

**SW1 up** (LEDR8 on), then:

```bash
py -3.13 python/push_model.py COM5 build/signed/p5_v2.bin
```

- [ ] `OK STATUS ... err=0 ... source=1`, then `VERSION model=2DDDA531 version=2`.
- [ ] **LEDR4 on, LEDR5 off** (source 1: flash slot A).
- [ ] It took a few seconds (upload, check, erase, write, read back).

### 3.6 Second update: 8-byte model, goes to the other slot

```bash
py -3.13 python/push_model.py COM5 build/signed/p8_v3.bin
```

- [ ] `OK ... source=2`, `VERSION model=C0F7E8B5 version=3`.
- [ ] **LEDR5 on, LEDR4 off** (slot B).

### 3.7 Rollback: an older version is refused

```bash
py -3.13 python/push_model.py COM5 build/signed/p5_v2.bin
```

- [ ] `ERROR COMMIT failed, code 7`.
- [ ] Still `model=C0F7E8B5 version=3`, LEDR5 on. The running model was never
  touched.

### 3.8 Boot from flash (KEY0)

Press **KEY0**. The FPGA restarts and has to find the model in its flash, check
the stamp, unscramble it and load it, all by itself.

```bash
py -3.13 python/push_model.py COM5
```

- [ ] `model=C0F7E8B5 version=3 ... source=2`. LEDR5 on.
- [ ] LEDR7 off (a reset clears it).

### 3.9 Wrong key: three strikes and the port locks

Push the rulebook signed with the wrong key **three times**:

```bash
py -3.13 python/push_model.py COM5 build/signed/wrongkey_v4.bin
```

- [ ] 1st: `ERROR COMMIT failed, code 12`.
- [ ] 2nd: code 12.
- [ ] 3rd: code 12, and **LEDR6 comes on** (locked).

Now push a good one, to check that even a good one is refused while locked:

```bash
py -3.13 python/push_model.py COM5 build/signed/p8_v3.bin
```

- [ ] `ERROR upload refused: ... err=2 ... locked=1`.

Then look at the counters and the model:

```bash
py -3.13 python/push_model.py COM5
```

- [ ] `COUNTERS ... sig_fails=3`.
- [ ] Still `model=C0F7E8B5 version=3`.

### 3.10 KEY0 does not unlock

Press **KEY0**, then:

```bash
py -3.13 python/push_model.py COM5
```

- [ ] Still `locked=1` (LEDR6 on). Only a power cycle clears the lockout.

### 3.11 Power cycle: the lockout clears and the model stays

The `.sof` design is lost when the power goes, so a power cycle here means:
unplug the DE10-Lite's USB, wait 5 seconds, plug it back in, and load the design
again (step 2.3). The flash keeps its contents through all of this.

```bash
py -3.13 python/push_model.py COM5
```

- [ ] `locked=0`, LEDR6 off.
- [ ] `model=C0F7E8B5 version=3 source=2`: the model survived the power cut.

### 3.12 Optional: keep the design in the FPGA for good (.pof)

Only if you want the board to start on its own at power on. Program the
`.pof` with **CFM only**:
1. Quartus > Tools > Programmer > Add File > `quartus\output_files\updatable_top.pof`.
2. Tick **Program/Configure** for **CFM0** (and any CFM row). **Leave UFM
   unticked**: ticking it erases the stored models.
3. Start.

Then unplug the USB, wait, and plug it in again, **without** loading a `.sof`:
- [ ] The LEDs show source 2 (LEDR5 on) by themselves.
- [ ] `push_model.py COM5` shows `C0F7E8B5 version=3`.

Don't use the bitstream encryption or JTAG security options in the
Programmer. They are hard to undo (`SECURITY_DESIGN.md`, section 3).

### 3.13 Put the switches back

SW1 down (maintenance off). Unplug the STM32's wires: GND last.

---

## Part 4: on the CAN bench (not at home)

These need the ESP32 bench and are in `docs/SPI_WIRING.md` (bench tests 5
and 6) and `docs/HANDOVER.md`:
- the four attacks with the model loaded over SPI (expected: every attack frame
  destroyed, 0 normal frames lost, as on 11-03);
- STM32 fw 17 on the four attacks: compare `lat`, `abort` and `dlate` with
  fw 16.

---

## Results to send back

| Step | Matched? | Notes (anything odd: error codes, LEDs) |
|---|---|---|
| 1.5 build, Fmax | | |
| 2.4 ROM model at power on | | |
| 3.3 first contact | | |
| 3.4 refused without SW1 | | |
| 3.5 first update, slot A | | |
| 3.6 second update, slot B | | |
| 3.7 rollback refused | | |
| 3.8 boot from flash after KEY0 | | |
| 3.9 lockout after 3 wrong keys | | |
| 3.10 KEY0 doesn't unlock | | |
| 3.11 power cycle: unlocked, model kept | | |
| 3.12 .pof (optional) | | |

## If something looks wrong

| Sign | Likely cause | What to do |
|---|---|---|
| `quartus_pgm -l` shows no USB-Blaster | driver | Device Manager > USB-Blaster > Update driver > Browse > `C:\altera_lite\25.1std\quartus\drivers\usb-blaster` |
| Quartus stops: `can't open device_key.vh` | step 1.3 not done | run step 1.3, compile again |
| LEDR4 and LEDR5 both off after loading | no model at all, or the board is still starting | wait 2 s; press KEY0. Still off: send me STATUS |
| LEDR7 on right at power on, source 3 | `err=18`: the flash did not answer | power cycle. If it stays, send me STATUS |
| LEDR0 to LEDR3 flicker with nothing connected | RX pin floating | step 2.1 |
| STATUS `FFFFFFFF` or not starting with `C1` | SPI wiring or GND | check the 5 wires, GND first; FPGA design loaded? |
| `err=1` | SW1 is down | SW1 up |
| `err=2` | locked out | power cycle (3.11) |
| `err=7` | version not higher than one already accepted | sign with a higher `--version` |
| `err=12` | stamp wrong: signed with another key, or the build used another `device_key.vh` | rebuild after 1.3, or sign with `C:\Users\Charbel\Desktop\master.key` |
| `err=16` | busy for a moment | try again |
| `ERROR no answer after COMMIT` | the FPGA got stuck or lost the design | send me STATUS, then load the `.sof` again |
| `ModuleNotFoundError: No module named 'serial'` | 1.1 not done | step 1.1 |
| No COM port after 3.1 | BT0 still on 3V3, or no RST | BT0 to GND, press RST |
