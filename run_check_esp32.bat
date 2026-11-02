@echo off
rem Double-click to run the whole CAN test: find the boards, check the link,
rem replay the dataset, open the results page.
rem Add options after the name when running from a terminal, for example:
rem   run_check_esp32.bat --file gear_dataset.csv --n 100000

set "PY=%~dp0..\.venv\Scripts\python.exe"
if not exist "%PY%" set "PY=python"

"%PY%" "%~dp0python\run_check.py" %*
echo.
pause
