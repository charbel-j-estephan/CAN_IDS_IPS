@echo off
rem Double-click: the prevention (IPS) test. Same as run_check_esp32.bat, but during
rem the replay the STM32 (stm32_ids fw 8 or later) destroys the frames its model
rem calls attacks. The ESP32 receiver records what survived on the bus. Needs all
rem three ESP32s (sender, attacker, receiver) and the STM32 on USB.

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%~dp0python\run_check.py" --ips %*
echo.
pause
