@echo off
rem Live demo of the CAN IDS/IPS (docs/DEMO.md). Double-click it.
rem   run_demo.bat prepare   before the talk: the STM32 becomes the update tool
rem   run_demo.bat           the demo: secure update, then attacks stopped
rem   run_demo.bat attacks   only the attack part
cd /d "%~dp0"
py -3.13 python\demo.py %*
echo.
pause
