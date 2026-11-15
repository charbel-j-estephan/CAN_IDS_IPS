@echo off
REM Puts a CAN IDS/IPS design on the DE10-Lite, for example after the board ran
REM Terasic's factory demo. Run it before run_demo.bat prepare.
REM
REM   program_fpga.bat           the updatable design (secure model updates over SPI)
REM   program_fpga.bat basic     the basic IDS: the 5-byte forest compiled into logic
REM   program_fpga.bat check     only check that Quartus, the files and the cable are there
REM
REM It programs the whole .pof: the design (CFM) and the user flash (UFM). The
REM UFM held the pushed models, so the updatable design then starts with the model
REM built into it (the same 5-byte model, LEDR4 and LEDR5 both on) and the demo
REM pushes a new, signed one. The updatable .pof holds the board's key: never share it.
setlocal
cd /d "%~dp0"

set "MODE=%~1"
set "POF_UPD=%~dp0updatable fpgas\quartus\output_files\updatable_top.pof"
set "POF_BASIC=%~dp0quartus_board\can_ids_top.pof"
set "POF=%POF_UPD%"
set "NAME=the updatable CAN IDS/IPS design"
if /i "%MODE%"=="basic" (
    set "POF=%POF_BASIC%"
    set "NAME=the basic IDS ^(forest compiled into logic^)"
)

if not exist "%POF%" (
    echo The design file is missing:
    echo   %POF%
    echo Build it once in Quartus: "updatable fpgas\quartus\updatable_top.qpf" ^(it needs
    echo keys\device_key.vh, see updatable fpgas\docs\SPI_WIRING.md^), or for the basic
    echo IDS "quartus_board\can_ids_top.qpf".
    goto :fail
)
if /i "%MODE%"=="check" if not exist "%POF_BASIC%" echo Note: the basic IDS file is missing: %POF_BASIC%

set "PGM="
if defined QUARTUS_ROOTDIR if exist "%QUARTUS_ROOTDIR%\bin64\quartus_pgm.exe" set "PGM=%QUARTUS_ROOTDIR%\bin64\quartus_pgm.exe"
if not defined PGM for /d %%D in ("C:\altera_lite\*" "C:\intelFPGA_lite\*" "C:\intelFPGA\*" "C:\altera\*") do (
    if exist "%%~D\quartus\bin64\quartus_pgm.exe" set "PGM=%%~D\quartus\bin64\quartus_pgm.exe"
)
if not defined PGM (
    echo Quartus Programmer not found. Install Quartus Prime Lite, or set QUARTUS_ROOTDIR.
    goto :fail
)

echo Programmer: %PGM%
echo Design:     %POF%
echo.
"%PGM%" -l 2>&1 | findstr /c:"USB-Blaster" >nul
if errorlevel 1 (
    echo No USB-Blaster found. Plug the DE10-Lite's USB cable into this PC
    echo ^(the USB-Blaster driver must be installed^) and run this again.
    goto :fail
)
echo USB-Blaster found.
if /i "%MODE%"=="check" (
    echo Check only: nothing was programmed.
    goto :end
)

echo.
echo This puts %NAME% on the DE10-Lite ^(about 30 seconds^).
choice /c YN /m "Program it"
if errorlevel 2 goto :end

"%PGM%" -m jtag -c 1 -o "p;%POF%" > "%TEMP%\program_fpga.log" 2>&1
findstr /c:"Successfully performed" "%TEMP%\program_fpga.log" >nul
if errorlevel 1 (
    type "%TEMP%\program_fpga.log" | findstr /i "error"
    echo.
    echo Programming FAILED. Full log: %TEMP%\program_fpga.log
    goto :fail
)
echo.
echo Programmed.
echo Now unplug the DE10-Lite's USB cable, plug it back, and check:
if /i "%MODE%"=="basic" goto :basic_leds
echo   LEDR4 and LEDR5 both on  = it runs the built-in model ^(right after programming^)
echo   SW0 and SW1 down
echo Then: run_demo.bat prepare
goto :end

:basic_leds
echo   SW0 up = LEDR9 on ^(prevention on^); LEDR0 blinks while frames arrive
echo   This design has no update port: SW1 does nothing.
echo Then: run_demo.bat run basic DoS
goto :end

:fail
echo.
pause
exit /b 1

:end
echo.
pause
exit /b 0
