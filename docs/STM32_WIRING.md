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

## Free pins used

PB8 and PB9 only. The board's LEDs (PA6, PA7), buttons (PE3, PE4, PA0),
USB (PA11, PA12) and SWD header are left alone.
