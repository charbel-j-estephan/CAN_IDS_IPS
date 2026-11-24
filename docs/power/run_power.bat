@echo off
rem Quartus Power Analyzer for both FPGA designs, with switching activity from
rem an Icarus simulation of real dataset frames on a simulated CAN bus.
rem   docs\power\hard.vcd   hardcoded design, hdl\frames_tb.v -DWITH_IPS, 20 ms of traffic
rem   docs\power\upd.vcd    updatable design, tb\core_frames_tb.v, 20 ms of traffic after boot
rem The script adds the power settings to each .qsf, runs quartus_pow, then puts
rem the original .qsf back. Reports are copied to docs\power.
setlocal
cd /d "%~dp0..\.."
set "ROOT=%CD%"
set "POW="
if defined QUARTUS_ROOTDIR if exist "%QUARTUS_ROOTDIR%\bin64\quartus_pow.exe" set "POW=%QUARTUS_ROOTDIR%\bin64\quartus_pow.exe"
if not defined POW for /d %%D in ("C:\altera_lite\*" "C:\intelFPGA_lite\*" "C:\intelFPGA\*" "C:\altera\*") do (
    if exist "%%~D\quartus\bin64\quartus_pow.exe" set "POW=%%~D\quartus\bin64\quartus_pow.exe"
)
if not defined POW (
    echo quartus_pow not found. Set QUARTUS_ROOTDIR.
    exit /b 1
)
echo Using %POW%

call :run "%ROOT%\quartus_board" can_ids_top "..\docs\power\hard.vcd" can_ids_top hard
call :run "%ROOT%\updatable fpgas\quartus" updatable_top "..\..\docs\power\upd.vcd" updatable_top upd
echo.
echo Done. Reports are in docs\power.
exit /b 0

:run
pushd "%~1"
set "V=%~3"
set "V=%V:\=/%"
if not exist "%~3" (
    echo Missing %~3, skipping %~2
    popd
    exit /b 0
)
copy /y %2.qsf %2.qsf.powbak >nul
>> %2.qsf echo.
>> %2.qsf echo set_global_assignment -name POWER_USE_INPUT_FILES ON
>> %2.qsf echo set_global_assignment -name POWER_INPUT_FILE_NAME "%V%" -section_id powvcd
>> %2.qsf echo set_instance_assignment -name POWER_READ_INPUT_FILE powvcd -to %4
>> %2.qsf echo set_global_assignment -name POWER_DEFAULT_TOGGLE_RATE "12.5%%"
>> %2.qsf echo set_global_assignment -name POWER_DEFAULT_INPUT_IO_TOGGLE_RATE "12.5%%"
>> %2.qsf echo set_global_assignment -name POWER_REPORT_SIGNAL_ACTIVITY ON
>> %2.qsf echo set_global_assignment -name POWER_REPORT_POWER_DISSIPATION ON
echo.
echo === %2 ===
"%POW%" %2 > "%ROOT%\docs\power\%5_quartus_pow.log" 2>&1
set "RC=%ERRORLEVEL%"
copy /y %2.qsf.powbak %2.qsf >nul
if exist output_files\%2.pow.summary (
    copy /y output_files\%2.pow.summary "%ROOT%\docs\power\%5.pow.summary" >nul
    copy /y output_files\%2.pow.rpt "%ROOT%\docs\power\%5.pow.rpt" >nul
)
if exist %2.pow.summary (
    copy /y %2.pow.summary "%ROOT%\docs\power\%5.pow.summary" >nul
    copy /y %2.pow.rpt "%ROOT%\docs\power\%5.pow.rpt" >nul
)
if exist "%ROOT%\docs\power\%5.pow.summary" type "%ROOT%\docs\power\%5.pow.summary"
echo quartus_pow exit code %RC%
popd
exit /b 0
