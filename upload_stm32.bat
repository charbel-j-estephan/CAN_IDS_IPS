@echo off
rem Double-click: upload stm32_ids to the STM32 without the BOOT0 jumper (needs
rem firmware fw 3 or later already on it; otherwise use the jumper once).

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%~dp0python\stm32_dfu.py" %*
echo.
pause
