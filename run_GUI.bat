@echo off
rem Double-click: one window to upload the STM32, build and program the FPGA,
rem run the FPGA simulation and the bench tests (python/bench_gui.py).

set "PY=%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe"
if not exist "%PY%" (
    echo Python for the bench was not found at:
    echo   %PY%
    echo It needs pyserial and numpy: pip install pyserial numpy
    pause
    exit /b 1
)
start "" "%PY:python.exe=pythonw.exe%" "%~dp0python\bench_gui.py"
