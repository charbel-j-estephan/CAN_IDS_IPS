@echo off
rem Double-click (or run from a terminal): the whole FPGA simulation.
rem   1. can_rx_tb.v: 1012 test frames, at 0 and +-3000 ppm sender clock error
rem   2. real dataset frames from the 4 dataset files (make_sim_frames.py)
rem   3. known_id.v on all 2048 IDs against the model's list
rem   4. those frames through can_rx + the forest (rf_ids.v) + the unknown ID
rem      rule, compared with Python frame by frame; the table opens in the browser
rem
rem   run_sim.bat              the 5-byte model (rf5_d9_p5_time, the original)
rem   run_sim.bat 6            the 6-byte model (rf5_d9_p6_time)
rem   run_sim.bat 8            the 8-byte model (rf5_d9_p8_time)
rem Other options go to make_sim_frames.py, e.g.:  run_sim.bat 6 --frames 1000
rem                                                run_sim.bat --files DoS_dataset.csv
setlocal
cd /d "%~dp0"
set "IV=C:\iverilog\bin"
if not exist build mkdir build

rem The first option may be the byte count; everything else is passed on.
set "FB=5"
set "ARGS="
:args
if "%~1"=="" goto :argsdone
if "%~1"=="5" (set "FB=5") else if "%~1"=="6" (set "FB=6") else if "%~1"=="8" (set "FB=8") else set "ARGS=%ARGS% %1"
shift
goto :args
:argsdone
set "MODEL=models/rf5_d9_p%FB%_time.joblib"
set "GEN=hdl/generated_p%FB%"
echo Model: %MODEL% (reads %FB% data bytes), forest: %GEN%/rf_ids.v
echo.

echo === 1. can_rx test frames ===
"%IV%\iverilog" -g2005 -I hdl/vectors -o build/can_rx_sim hdl/can_rx_tb.v hdl/can_rx.v || goto :fail
for %%d in (0 3000 -3000) do (
    echo --- sender clock off by %%d ppm
    "%IV%\vvp" -n build/can_rx_sim +drift_ppm=%%d | findstr /r "^frames: ^PASS ^FAIL ^ERROR"
)

echo.
echo === 2. dataset frames ===
if not exist %GEN%\rf_ids.v (
    py -3.13 python/export_verilog.py --model %MODEL% --out %GEN% || goto :fail
)
py -3.13 python/make_sim_frames.py --model %MODEL% %ARGS% || goto :fail

echo.
echo === 3. known_id: all 2048 IDs against the model's list ===
"%IV%\iverilog" -g2005 -o build/known_id_sim hdl/known_id_tb.v hdl/known_id.v || goto :fail
"%IV%\vvp" -n build/known_id_sim | findstr /r "^ID ^PASS ^FAIL"

echo.
echo === 4. dataset frames through can_rx, the forest and the unknown ID rule ===
"%IV%\iverilog" -g2005 -DWITH_RF -DFEAT_BYTES=%FB% -I build/sim -o build/frames_sim hdl/frames_tb.v hdl/can_rx.v %GEN%/rf_ids.v hdl/known_id.v || goto :fail
"%IV%\vvp" -n build/frames_sim | findstr /v "finish called"
py -3.13 python/compare_sim.py || goto :fail
start "" "build\sim\compare.html"
goto :end

:fail
echo.
echo Stopped: the step above failed.
:end
echo.
pause
