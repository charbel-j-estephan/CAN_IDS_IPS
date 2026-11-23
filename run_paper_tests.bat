@echo off
rem The STM32 bench tests for the paper revision (no FPGA, nothing to flash).
rem Needs the three ESP32s (sender, attacker v2, receiver) and the STM32
rem (stm32_ids, fw 16 or 17) on USB, the bus wired as usual.
rem Every test saves its own folder in runs\. About 30 to 40 minutes in all.
rem
rem   1. Held out traffic, prevention on: all four files mixed, from the part of
rem      each file the model never saw. The attacker comes back from bus off at
rem      every block, so it also shows recovery. Two runs, 40,000 frames each.
rem   2. Prevention off for Fuzzy, gear and RPM (DoS is already in Table IV).
rem   3. False flag test: genuine frames the 5 byte model wrongly flags, no
rem      attacker. Shows whether the real sender gets pushed to bus off.
rem   4. One longer held out run, 200,000 frames.
rem
rem Results: docs\NEW_RESULTS_2026-11-21.md says what to look for in each.

setlocal
set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"
cd /d "%~dp0python"

echo === 1a. Held out mixed, IPS on, 40000 frames, seed 1 ===
"%PY%" run_check.py --ips --file mixed --n 40000 --seed 1
echo === 1b. Held out mixed, IPS on, 40000 frames, seed 2 ===
"%PY%" run_check.py --ips --file mixed --n 40000 --seed 2 --skip-check

echo === 2. Prevention off ===
"%PY%" run_check.py --file Fuzzy_dataset.csv --n 5000 --skip-check
"%PY%" run_check.py --file gear_dataset.csv --n 5000 --skip-check
"%PY%" run_check.py --file RPM_dataset.csv --n 5000 --skip-check

echo === 3. False flag test, no attacker ===
"%PY%" fp_test.py

echo === 4. Held out mixed, IPS on, 200000 frames ===
"%PY%" run_check.py --ips --file mixed --n 200000 --seed 3 --skip-check

echo.
echo All done. The run folders are in runs\ (newest last).
pause
