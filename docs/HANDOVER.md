# Handover: CAN test bench status

Branch `claude/esp32-can-bus-wiring-372c60`, worktree
`C:\Users\Charbel\Desktop\CAN_IDS_IPS\.claude\worktrees\esp32-can-bus-wiring-372c60`.
Everything is committed locally. Nothing is pushed and nothing is merged into `main`.

## Current goal

Send the Car Hacking dataset from one ESP32 to another over a real CAN bus, with
no IDS and no training. The STM32 gateway and the model are parked.

## What works (verified)

| Item | Evidence |
|---|---|
| Plain CAN link between two ESP32s (`link_sender`, `link_receiver`) | Serial Monitor showed frames 1 to 7 arriving with the counter going up |
| STM32 blink project builds and uploads over USB | Needs the **libusbK** driver in Zadig, USB DFU, BOOT0 jumper |
| STM32 gateway firmware compiles | 200 KB of 512 KB flash with the real model |
| Model trained on the real dataset | 99.98 % held-out accuracy (300k frames per file sample) |
| `send_dataset.py` | Tested only against simulated boards, not real hardware |

The LED blink on the STM32 was never confirmed (no LED reported blinking after
upload). The USB upload step itself worked once libusbK was installed.

## Not yet tested on hardware

- `esp32/sender_twai`, `esp32/passthrough_twai`: never compiled or flashed
- `python/send_dataset.py` on the real bus
- `mcu/stm32_gateway`: never loaded on the board, no second CAN transceiver wired
- `python/esp32_eval.py`: the MCP2515 and model scoring paths

## Blocker right now

Only one ESP32 shows up as a COM port (a CH9102 on COM7 or COM8; the number
changes when replugged). The second board does not appear: probably a
charge-only cable or a missing USB driver. The user has two data cables and one
power-only cable. Test each cable with the board alone and run:

```
python -c "import serial.tools.list_ports as lp; [print(p.device, p.description) for p in lp.comports()]"
```

## How to resume

1. Get both ESP32s showing as COM ports.
2. Flash `esp32/sender_twai` to A and `esp32/passthrough_twai` to B (Arduino IDE,
   ESP32 core 3.2.0, board "ESP32 Dev Module"). Close the Serial Monitors.
3. From `python/` in the project venv (needs `pyserial` and `numpy`):
   `python send_dataset.py --sender COMx --receiver COMy`
4. Expected: `Arrived intact ... N of N`. If frames are lost, raise `GAP_US` in
   the sender.

## Hardware facts

- ESP32 modules: transceiver only, pins TX, RX, VIN, GND, CANH, CANL.
  Wiring: TX to GPIO 5, RX to GPIO 4, 120 ohm termination on both ends.
- STM32: black F407VET6 board, 8 MHz crystal, no on-board debugger. Mini-USB is
  power plus DFU only. No ST-LINK owned. ST blocks st.com downloads in the
  user's country, so STM32CubeIDE is not an option: use PlatformIO in VS Code.
- STM32 USB upload: fit the BOOT0 jumper (BT0 to 3V3), plug in, press RST,
  Upload from `mcu/platformio_blink` or `mcu/stm32_gateway`, then remove the
  jumper and press RST. The Zadig driver must be libusbK, not WinUSB, and a
  USB port that worked (some ports fail with Code 43).
- STM32 CAN pins: CAN1 on PB8/PB9, CAN2 on PB12/PB13 (PA11/PA12 are USB).

## Repo map for this work

| Path | Purpose |
|---|---|
| `esp32/link_sender`, `esp32/link_receiver` | plain CAN test, Serial Monitor only |
| `esp32/sender_twai`, `esp32/passthrough_twai` | dataset streaming, no model |
| `esp32/sender`, `esp32/receiver`, `esp32/receiver_twai` | MCP2515 and IDS variants, parked |
| `python/send_dataset.py` | send dataset, check frames arrive intact |
| `python/esp32_eval.py` | scoring for IDS and gateway runs |
| `mcu/platformio_blink` | STM32 blink test |
| `mcu/stm32_gateway` | CAN1 to model to CAN2 gateway, parked |
| `docs/ESP32_WIRING.md` | wiring for every variant |
| `data/car_hacking/`, `models/` | local only, git-ignored |

## Open decisions

- Merge the branch into `main`, or keep working in the worktree.
- Whether to revisit the STM32 gateway and IDS later.
