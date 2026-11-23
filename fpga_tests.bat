@echo off
rem FPGA tests for the paper. Open docs\FPGA_TEST_GUIDE.html first: it says
rem which switches to set before each step and what to write down after.
rem
rem   fpga_tests.bat check      boards, cable, Quartus and the design file
rem   fpga_tests.bat t1         quick check, DoS 5,000 frames          SW0 up, SW2 down
rem   fpga_tests.bat t2         held out mixed, 40,000, majority vote  SW0 up, SW2 down
rem   fpga_tests.bat t3         held out mixed, 40,000, strict vote    SW0 up, SW2 UP
rem   fpga_tests.bat t4         false flag test (no attacker frames)   SW0 up, SW2 as the guide says
rem   fpga_tests.bat t5         lost frames, STM32 records too         SW0 up, SW2 down
rem   fpga_tests.bat t6         live update to the 6 byte model        SW1 UP
rem   fpga_tests.bat counters   FPGA counters (STM32 = update tool)   SW1 UP
rem
rem Every replay saves a folder in runs\ named by date and time. Write that name
rem in the guide's result box.

setlocal
cd /d "%~dp0"
rem Replays use the bench's Python (as run_check_esp32.bat does). The update
rem steps use py -3.13 (as run_demo.bat and the home test guide do).
set PY=py -3.13
if exist "%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe" set PY="%USERPROFILE%\Desktop\esp32 setup\.venv\Scripts\python.exe"
if exist "%~dp0..\.venv\Scripts\python.exe" set PY="%~dp0..\.venv\Scripts\python.exe"
set UPY=py -3.13

set "STEP=%~1"
if "%STEP%"=="" goto :usage

for %%s in (check t1 t2 t3 t4 t5 t6 counters) do if /i "%STEP%"=="%%s" goto :%%s
goto :usage

:check
%PY% python\run_check.py --dry-run
call program_fpga.bat check
goto :end

:t1
%PY% python\run_check.py --file DoS_dataset.csv --n 5000
goto :end

:t2
%PY% python\run_check.py --file mixed --n 40000 --seed 1 --recover
goto :end

:t3
%PY% python\run_check.py --file mixed --n 40000 --seed 1 --recover --skip-check
goto :end

:t4
%PY% python\fp_test.py --fpga
goto :end

:t5
%PY% python\run_check.py --listen --file RPM_dataset.csv --n 5000
goto :end

:t6
pushd "updatable fpgas\python"
%UPY% push_p6.py
popd
goto :end

:counters
pushd "updatable fpgas\python"
for /f "delims=" %%p in ('%UPY% -c "from serial.tools import list_ports as l;print(*[p.device for p in l.comports() if p.vid==0x0483])"') do %UPY% push_model.py %%p
popd
goto :end

:usage

if /i "%STEP%"=="check" (
    %PY% python\run_check.py --dry-run
    call program_fpga.bat check
    goto :end
)
if /i "%STEP%"=="t1" (
    %PY% python\run_check.py --file DoS_dataset.csv --n 5000
    goto :end
)
if /i "%STEP%"=="t2" (
    %PY% python\run_check.py --file mixed --n 40000 --seed 1 --recover
    goto :end
)
if /i "%STEP%"=="t3" (
    %PY% python\run_check.py --file mixed --n 40000 --seed 1 --recover --skip-check
    goto :end
)
if /i "%STEP%"=="t4" (
    %PY% python\fp_test.py --fpga
    goto :end
)
if /i "%STEP%"=="t5" (
    %PY% python\run_check.py --listen --file RPM_dataset.csv --n 5000
    goto :end
)
if /i "%STEP%"=="t6" (
    pushd "updatable fpgas\python"
    %UPY% push_p6.py
    popd
    goto :end
)
if /i "%STEP%"=="counters" (
    pushd "updatable fpgas\python"
    for /f "delims=" %%p in ('%UPY% -c "from serial.tools import list_ports as l;print(*[p.device for p in l.comports() if p.vid==0x0483])"') do %UPY% push_model.py %%p
    popd
    goto :end
)

:usage
echo Usage: fpga_tests.bat check ^| t1 ^| t2 ^| t3 ^| t4 ^| t5 ^| t6 ^| counters
echo See docs\FPGA_TEST_GUIDE.html

:end
echo.
pause
