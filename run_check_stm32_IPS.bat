@echo off
rem Double-click: the STM32 IPS test (prevention). During the replay the STM32
rem (stm32_ids fw 8 or later) destroys the frames its model calls attacks; the
rem ESP32 receiver records what survived on the bus. Needs all three ESP32s
rem (sender, attacker, receiver) and the STM32 on USB.
rem
rem By default it plays all four dataset files mixed: blocks of frames from
rem each file in random order, from the part the model was not trained on. The
rem attacker comes back on the bus at every block, so needs attack_sender
rem version 2 (it prints READY ATTACKER 2).
rem   run_check_stm32_IPS.bat                         20,000 frames, all four files
rem   run_check_stm32_IPS.bat --n 40000               more frames
rem   run_check_stm32_IPS.bat --seed 123              the same blocks as an earlier run
rem   run_check_stm32_IPS.bat --file RPM_dataset.csv  one file only (its first frames)

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%~dp0python\run_check.py" --ips --file mixed %*
echo.
pause
