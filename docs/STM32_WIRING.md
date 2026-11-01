# STM32 IDS node: wiring

The black STM32F407VET6 board joins the CAN bench as one more node. It reads
the bus bit by bit in software (M1), classifies every frame (M2) and later
destroys attack frames with an error flag (M3).

```
PC --USB--> sender    replay_sender  (normal frames) --+
PC --USB--> attacker  attack_sender  (attack frames) --+
PC --USB--> receiver  usb_receiver   (logger, ACKs)  --+== CAN bus (CANH / CANL)
PC --USB--> STM32     IDS            (listens)       --+
```

Keep the receiver on the bus. The STM32 only listens (its TX stays recessive),
so it does not acknowledge frames. With the receiver there, every frame is
still ACKed, even in the 2-board link check where the attacker is absent, and the
receiver's log is the reference to check the STM32's decoding against. Short
of USB ports: the receiver may run from any USB charger or power bank, it only
needs power to ACK.

## STM32 to its own CAN module (TJA1050)

Use a fourth CAN module, a TJA1050 like the others.

| TJA1050 module | STM32 black F407VE | Note |
|---|---|---|
| RX | **PB8** | the bus as the STM32 reads it: 5 V = recessive, 0 V = dominant |
| TX | **PB9** | released (recessive) by the firmware; M3 pulls it low |
| VCC | **5V** header pin | the TJA1050 needs 4.75 to 5.25 V |
| GND | GND | |
| CANH / CANL | bus rails | like the other modules |

No divider on RX (unlike the ESP32): PB8 and PB9 are 5 V tolerant ("FT" in
the F407 datasheet pin table), so the module's 5 V RX goes straight to PB8.
Power the module from the STM32 board itself, so the STM32 is always powered
whenever 5 V reaches its pins.

TX: the TJA1050's TXD has its own pull up to 5 V, and 3.3 V is below its
guaranteed high level (0.7 x VCC = 3.5 V). So the firmware drives PB9 as
**open drain with no internal pull up**: released = the module pulls it to 5 V
(recessive), low = dominant. Never push pull on PB9, and never the internal
pull up with 5 V on the pin.

PB8 and PB9 are also the F407's CAN1 RX/TX pins (alternate function 9), so the
same wiring works if the hardware CAN controller is ever used for comparison.

Power: the board runs from its micro USB socket (the same one the PC talks to
over USB CDC and DFU). Measure its 5V header pin first: it should read 4.75 V
or more. If it is lower (a diode on the board's USB supply), power the module
from the receiver's VIN pin instead, grounds joined.

## The bus

- CANH to the CANH rail, CANL to the CANL rail, GND to the common GND rail.
- Only the two modules at the physical ends of the bus keep their 120 ohm.
  TJA1050 modules usually have one soldered on (marked 121, often R2). If the
  STM32's module sits in the middle, remove its resistor (or put the STM32's
  module at one end and remove the resistor from the module that is now in
  the middle).
- Power off, multimeter CANH to CANL: about **60 ohm**. 40 ohm means three
  terminators are fitted; remove one.

## Upload M1 (software CAN receiver)

The firmware is `mcu/stm32_ids` (in the bench folder: `stm32_ids`), a
PlatformIO project.

1. Close every Arduino Serial Monitor.
2. VS Code: File > Open Folder > `stm32_ids`. PlatformIO sets it up.
3. BOOT0 jumper: BT0 to 3V3 (BT1 stays on GND), press RST. Windows now shows
   a DFU device, not a COM port. Zadig driver for it: libusbK.
4. PlatformIO: Upload (the arrow in the bottom bar).
5. BT0 back to GND, press RST. The STM32 now shows up as a new COM port
   (USB serial, ST's 0483:5740).

Check before the first run:

- All boards powered and the bus idle: PB8 reads about 5 V (recessive), and
  so does PB9 (pulled up by the module).
- LED D2 (PA6) lights while frames arrive.

## Test M1

1. First, with the STM32 wired in and running: the usual `run_check.bat`.
   The ESP32 receiver must still get 20,000 of 20,000. That shows the STM32's
   module leaves the bus alone, so any problem in step 2 is the decoding, not
   the wiring.
2. Then `run_check_stm32.bat` (or `run_check.bat --stm32`): the same test,
   but the frames checked are the ones the STM32 decoded.

In step 2:

- the link check (300 frames back to back) must pass, 300 of 300;
- the replay must deliver 100%, like the ESP32 receiver.

The receiver line on the results page shows the STM32's counters: `rx`
frames sent to the PC, `ovf` frames lost because its buffer was full,
`stuff`, `form` and `crc` frames thrown away for errors, `other` extended or
remote frames (none in the datasets). All but `rx` should stay 0. If step 2
fails, the script's advice about wiring (60 ohm and so on) does not apply when
step 1 passed: look at those counters instead.

A miswired STM32 module (TX stuck low) holds the bus dominant and every board
shows errors: unplug the STM32's module first when that happens.

## M2: the model on the STM32

From fw 5 the STM32 also classifies every frame (5 trees, depth 9, ID + DLC +
first 5 data bytes, plus the unknown ID rule), as soon as the 5th data byte is
in, while the rest of the frame is still on the bus. `run_check_stm32.bat`
then prints, after the usual results:

- recall per class, attack frames flagged, false alarms
- frames with no decision by the end of the frame (must be 0)
- how long after the 5th data byte the decision came, and how much of the
  frame was still left: the time M3 has to send an error flag

The model comes from `python/train.py` and `python/export_c.py --out
mcu/stm32_ids/src/can_ids_model.h`; READY and #STAT show its id.
`python/check_stm32_model.py --run <run folder>` (repo `.venv`) checks that
the STM32 gave exactly the class Python gives, frame by frame. Sending 'c' to
the STM32's port switches the model off ('C' on), to see whether running it
disturbs the bit timing (#STAT `lat`, `nlate`, `stuff`, `crc`).

## Free pins used

PB8 and PB9 only. The board's LEDs (PA6, PA7), buttons (PE3, PE4, PA0),
USB (PA11, PA12) and SWD header are left alone.

## M3: prevention (IPS), fw 8

Prevention is **off at power up**. Only the IPS test switches it on, for the
replay, and off again at the end (also if the script stops early).

When the model calls a frame an attack, the STM32 sends an **error flag**:
PB9 (the module's TX) dominant for 6 bit times, starting at the next bit
boundary. Every node then throws the frame away; the attacker's controller
retransmits it and its transmit error counter rises by 8 each time, so a
persistent attacker goes error passive (127) and then bus off (255). PB9 is
TIM4 channel 4, the timer that does the bit timing, so the flag starts and
ends on the exact tick in hardware. A verdict that comes after the end of
frame field has begun is too late: the frame stands and it counts as `ilate`.
The replay's start marker (ID 0x7FF, "CANSTART") is on an allow list and is
never destroyed. PB9 is never held dominant for more than 8 bits (`abort`).

Test, with all three ESP32s and the STM32 on USB:

1. `run_check_stm32.bat` first (prevention stays off): the STM32 must decode
   100% with `crc`, `stuff` and `dlate` at 0, as in the M1 and M2 tests.
2. Then `run_check_ips.bat` (or `run_check.bat --ips`). The link check runs
   without prevention; the replay runs with it. The ESP32 receiver is the
   record of what survived on the bus.

Read the result:

- attack frames destroyed: the share of attack frames the receiver never saw.
  Once the attacker is bus off its remaining frames never reach the bus at
  all, so this counts both destroyed and silenced frames.
- normal frames lost: must stay 0. Anything else is a false block (or a frame
  lost on the bus).
- STM32 `rej` error flags sent, `ilate` attack verdicts that came too late,
  `allow` (the marker), `abort` (must be 0), `dlate` no decision in time.
- attacker `state` (busoff expected on DoS) and `tec`.

STM32 commands over its USB port: `I` prevention on, `i` off, `Z` zero the
counters, `c`/`C` model off/on, `f`/`F` and `p`/`P` timing diagnostics.
