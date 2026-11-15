@echo off
REM Puts the CAN IDS/IPS design (the updatable FPGA) back on the DE10-Lite,
REM for example after the board ran Terasic's factory demo. Run it before
REM run_demo.bat prepare.
REM
REM   program_fpga.bat          program the board
REM   program_fpga.bat check    only check that Quartus, the file and the cable are there
REM
REM It programs the whole .pof: the design (CFM) and the user flash (UFM). The
REM UFM held the pushed models, so the board then starts with the model built
REM into the design (the same 5-byte model, LEDR4 and LEDR5 both on) and the
REM demo pushes a new, signed one. The .pof holds the board's key: never share it.
setlocal
cd /d "%~dp0"

set "POF=%~dp0updatable fpgas\quartus\output_files\updatable_top.pof"
if not exist "%POF%" (
    echo The design file is missing:
    echo   %POF%
    echo Build it once: open "updatable fpgas\quartus\updatable_top.qpf" in Quartus
    echo and compile (it needs keys\device_key.vh, see updatable fpgas\docs\SPI_WIRING.md^).
    goto :fail
)

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
if /i "%~1"=="check" (
    echo Check only: nothing was programmed.
    goto :end
)

echo.
echo This replaces what the DE10-Lite runs now ^(about 30 seconds^).
choice /c YN /m "Program the CAN IDS/IPS design"
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
echo   LEDR4 and LEDR5 both on  = it runs the built-in model ^(right after programming^)
echo   SW0 and SW1 down
echo Then: run_demo.bat prepare
goto :end

:fail
echo.
pause
exit /b 1

:end
echo.
pause
exit /b 0
