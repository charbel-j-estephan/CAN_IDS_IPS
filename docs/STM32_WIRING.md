# STM32 IDS node: wiring

The black STM32F407VET6 board joins the CAN bench as one more node. It reads
the bus bit by bit in software (M1), classifies every frame (M2) and later
destroys attack frames with an error flag (M3).

```
PC --USB--> ESP32 A  replay_sender  (normal frames) --+
PC --USB--> ESP32 C  attack_sender  (attack frames) --+
PC --USB--> ESP32 B  usb_receiver   (logger, ACKs)  --+== CAN bus (CANH / CANL)
PC --USB--> STM32    IDS            (listens)       --+
```

Keep ESP32 B on the bus. The STM32 only listens (its TX stays recessive), so
it does not acknowledge frames. With B there, every frame is still ACKed, even
in the 2-board link check where C is absent, and B's log is the reference to
check the STM32's decoding against. Short of USB ports: B may run from any USB
charger or power bank, it only needs power to ACK.

## STM32 to its own CAN module

Use a fourth CAN module, same kind as the others.

| CAN module | STM32 black F407VE | Note |
|---|---|---|
| RX (R, CRX) | **PB8** | the bus as the STM32 reads it, high = recessive |
| TX (D, CTX) | **PB9** | held high (recessive) by the firmware; M3 pulls it low |
| VCC | **3V3** for an SN65HVD230, **5V** for a TJA1050 / TJA1051 | |
| GND | GND | |

PB8 and PB9 are also the F407's CAN1 RX/TX pins (alternate function 9), so the
same wiring works if the hardware CAN controller is ever used for comparison.
Both pins are 5 V tolerant (FT), so a 5 V module's RX can go straight to PB8,
no divider needed (unlike the ESP32).

TX with a 5 V module (TJA1050): the TJA's TXD has its own pull up to 5 V, and
3.3 V is below its guaranteed high level. The firmware therefore drives PB9 as
open drain with no internal pull up: released = the TJA pulls it to 5 V
(recessive), low = dominant. With an SN65HVD230 (3.3 V) PB9 is a normal push
pull output held high.

Power: the board runs from its micro USB socket (the same one the PC talks to
over USB CDC and DFU). Take 3V3 / 5V / GND for the module from the board's
header pins.

## The bus

- CANH to the CANH rail, CANL to the CANL rail, GND to the common GND rail.
- Only the two modules at the physical ends of the bus keep their 120 ohm.
  Many SN65HVD230 boards have a 120 ohm resistor soldered on (often marked
  R2 / 121). If the STM32's module sits in the middle and has one, remove it
  (or put the STM32's module at one end and remove the resistor from the
  module that is now in the middle).
- Power off, multimeter CANH to CANL: about **60 ohm**. 40 ohm means three
  terminators are fitted; remove one.

## Before the first upload

1. Close every Arduino Serial Monitor.
2. Upload over USB DFU: BOOT0 jumper (BT0 to 3V3), press RST, Upload in
   PlatformIO, remove the jumper, press RST. Zadig driver for the DFU device:
   libusbK.
3. With all boards powered and idle: PB8 reads about 3.3 V (recessive). With
   a 5 V module and the multimeter on PB8: about 5 V is also fine (FT pin).
4. Run `run_check.bat` once as before: still 20,000 of 20,000 with the STM32
   connected. A miswired STM32 module (TX stuck low) holds the bus dominant and
   every board shows errors: unplug the STM32's module first when that happens.

## Free pins used

PB8 and PB9 only. The board's LEDs (PA6, PA7), buttons (PE3, PE4, PA0),
USB (PA11, PA12) and SWD header are left alone.
