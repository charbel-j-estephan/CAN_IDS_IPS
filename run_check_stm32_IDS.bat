@echo off
rem Double-click: the STM32 IDS test (detection only, no prevention). The
rem frames checked are the ones the STM32 IDS node (stm32_ids) decodes; the ESP32
rem receiver must still be on the bus to acknowledge frames.
rem
rem By default it plays all four dataset files mixed: blocks of frames from
rem each file in random order, from the part the model was not trained on.
rem   run_check_stm32_IDS.bat                         20,000 frames, all four files
rem   run_check_stm32_IDS.bat --n 40000               more frames
rem   run_check_stm32_IDS.bat --seed 123              the same blocks as an earlier run
rem   run_check_stm32_IDS.bat --file RPM_dataset.csv  one file only (its first frames)

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%~dp0python\run_check.py" --stm32 --file mixed %*
echo.
pause
