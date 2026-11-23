@echo off
rem STM32 tests for the paper. Open docs\STM32_TEST_GUIDE.html first: it says
rem what to check before each step and what to write down after.
rem
rem   stm32_tests.bat check   boards and ports
rem   stm32_tests.bat t1      held out mixed, 40,000 frames, seed 1, prevention on
rem   stm32_tests.bat t2      held out mixed, 40,000 frames, seed 2, prevention on
rem   stm32_tests.bat t3      prevention off: Fuzzy, gear, RPM (5,000 frames each)
rem   stm32_tests.bat t4      false flag test, genuine frames only, no attacker
rem   stm32_tests.bat t5      long held out mixed run, 200,000 frames
rem   stm32_tests.bat all     t1 to t5 in a row (about 40 minutes)
rem
rem Every replay saves a folder in runs\ named by date and time.

setlocal
cd /d "%~dp0"
set PY=py -3.13
if exist "%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe" set PY="%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe"
if exist "%~dp0..\.venv\Scripts\python.exe" set PY="%~dp0..\.venv\Scripts\python.exe"

set "STEP=%~1"
for %%s in (check t1 t2 t3 t4 t5 all) do if /i "%STEP%"=="%%s" goto :%%s
goto :usage

:check
%PY% python\run_check.py --dry-run
goto :end

:t1
%PY% python\run_check.py --ips --file mixed --n 40000 --seed 1
goto :end

:t2
%PY% python\run_check.py --ips --file mixed --n 40000 --seed 2 --skip-check
goto :end

:t3
%PY% python\run_check.py --file Fuzzy_dataset.csv --n 5000
%PY% python\run_check.py --file gear_dataset.csv --n 5000 --skip-check
%PY% python\run_check.py --file RPM_dataset.csv --n 5000 --skip-check
goto :end

:t4
%PY% python\fp_test.py
goto :end

:t5
%PY% python\run_check.py --ips --file mixed --n 200000 --seed 3 --skip-check
goto :end

:all
%PY% python\run_check.py --ips --file mixed --n 40000 --seed 1
%PY% python\run_check.py --ips --file mixed --n 40000 --seed 2 --skip-check
%PY% python\run_check.py --file Fuzzy_dataset.csv --n 5000 --skip-check
%PY% python\run_check.py --file gear_dataset.csv --n 5000 --skip-check
%PY% python\run_check.py --file RPM_dataset.csv --n 5000 --skip-check
%PY% python\fp_test.py
%PY% python\run_check.py --ips --file mixed --n 200000 --seed 3 --skip-check
goto :end

:usage
echo Usage: stm32_tests.bat check ^| t1 ^| t2 ^| t3 ^| t4 ^| t5 ^| all
echo See docs\STM32_TEST_GUIDE.html

:end
echo.
pause
