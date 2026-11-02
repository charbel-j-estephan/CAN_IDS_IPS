@echo off
rem Double-click: the same test as run_check_esp32.bat, but the frames checked are the
rem ones the STM32 IDS node (stm32_ids) decodes. The ESP32 receiver must still be
rem on the bus to acknowledge frames.

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%~dp0python\run_check.py" --stm32 %*
echo.
pause
