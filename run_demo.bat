@echo off
rem Live demo of the CAN IDS/IPS, one step at a time. Double-click it.
rem Before: the bench wired and powered (docs/DEMO.md), the FPGA programmed
rem (it keeps its model in flash), the STM32 with the IDS firmware on the bus,
rem SW1 down on the DE10-Lite. Each step tells you which switch to set, waits
rem for a key, replays real car traffic with an attack and opens the results
rem page. Close the browser tab and press a key in this window to go on.

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"
set "RC=%~dp0python\run_check.py"

echo.
echo ================================================================
echo  CAN intrusion detection and prevention: live demo
echo ================================================================
echo.
echo  Checking which boards are plugged in...
"%PY%" "%RC%" --dry-run
if errorlevel 1 goto :boards
echo.

echo ----------------------------------------------------------------
echo  STEP 1  No protection
echo  On the DE10-Lite: SW0 DOWN (LEDR9 off).
echo  A DoS attack is replayed with real car traffic. Expect: every
echo  attack frame reaches the car's network.
echo ----------------------------------------------------------------
pause
"%PY%" "%RC%" --file DoS_dataset.csv --n 5000

echo.
echo ----------------------------------------------------------------
echo  STEP 2  The FPGA protects the bus
echo  On the DE10-Lite: SW0 UP (LEDR9 on).
echo  Same DoS attack. Expect: every attack frame destroyed on the
echo  wire, 0 normal frames lost, the attacker forced off the bus.
echo ----------------------------------------------------------------
pause
"%PY%" "%RC%" --file DoS_dataset.csv --n 5000 --skip-check

echo.
echo ----------------------------------------------------------------
echo  STEP 3  A harder attack: spoofed engine speed (RPM)
echo  Keep SW0 UP. The attacker sends frames with a real car ID, so
echo  only the content gives them away. Expect: all destroyed.
echo ----------------------------------------------------------------
pause
"%PY%" "%RC%" --file RPM_dataset.csv --n 5000 --skip-check

echo.
echo ----------------------------------------------------------------
echo  STEP 4  The STM32 protects the bus instead (software, 168 MHz)
echo  On the DE10-Lite: SW0 DOWN (LEDR9 off), the FPGA only listens.
echo  Expect: every attack frame destroyed by the STM32.
echo ----------------------------------------------------------------
pause
"%PY%" "%RC%" --ips --file DoS_dataset.csv --n 5000 --skip-check

echo.
echo ================================================================
echo  Demo done. Every run is saved in runs\ with its results page.
echo ================================================================
pause
exit /b 0

:boards
echo.
echo  The boards were not all found: check the USB cables (three ESP32s
echo  and the STM32) and close any Arduino Serial Monitor, then run
echo  this again. Wiring: docs\DEMO.md.
pause
exit /b 1
