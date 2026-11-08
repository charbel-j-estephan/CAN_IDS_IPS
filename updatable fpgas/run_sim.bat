@echo off
rem Every simulation of the updatable model. Takes about an hour.
rem   1. exports the 5, 6 and 8 byte blobs and the test blobs
rem   2. engine latency and throughput
rem   3. every distinct dataset frame vs Python, blob loaded through SPI and COMMIT
rem   4. update path: rejections, power cuts, boot, lockout (development build)
rem   5. the same with READ of the model blocked (production build)
rem   6. the whole design on a simulated bus, IPS on
rem   7. the model swapped from 5 to 8 bytes in the middle of traffic
rem
rem Needs Icarus in C:\iverilog\bin and py -3.13 with scikit-learn. The models
rem and the dataset are read from models\ and data\ of the repo.
setlocal
cd /d "%~dp0"
set "IV=C:\iverilog\bin"
if not exist build mkdir build
set "RTL=rtl/engine/sdp_ram.v rtl/engine/tree_level.v rtl/engine/rf_engine.v rtl/engine/rf_classifier.v rtl/frontend_can/can_features.v rtl/frontend_can/can_rx.v rtl/frontend_can/ips_flag.v rtl/update/crc32.v rtl/update/spi_slave.v rtl/update/cmd_decoder.v rtl/update/blob_walker.v rtl/update/blob_reader.v rtl/update/update_core.v rtl/nv/nv_ufm_avmm.v rtl/nv/boot_rom.v rtl/top/updatable_core.v"

echo === 1. blobs ===
for %%b in (5 6 8) do (
    for /f "delims=" %%m in ('py -3.13 -c "import sys;sys.path.insert(0,'python');import paths;print(paths.find_shared('models','rf5_d9_p%%b_time.joblib'))"') do (
        py -3.13 python/export_blob.py --model "%%m" --out build/blobs/p%%b || goto :fail
    )
)
py -3.13 python/make_test_blobs.py || goto :fail
copy /y build\blobs\p5\model.hex build\default_model.hex >nul

echo.
echo === 2. latency and throughput ===
"%IV%\iverilog" -g2005 -o build/latency_sim tb/latency_tb.v rtl/engine/sdp_ram.v rtl/engine/tree_level.v rtl/engine/rf_engine.v rtl/engine/rf_classifier.v || goto :fail
pushd build & "%IV%\vvp" -n latency_sim +blob=blobs/p5/model.hex | findstr /r "^latency ^back PASS FAIL" & popd

echo.
echo === 3. every dataset frame, blob over SPI ===
py -3.13 python/accuracy_engine.py --spi --bytes 5 6 8 || goto :fail

echo.
echo === 4. update path, development build ===
"%IV%\iverilog" -g2005 -I tb -o build/update_sim tb/update_tb.v tb/ufm_model.v %RTL% || goto :fail
pushd build & "%IV%\vvp" -n update_sim | findstr /r "^PASS ^FAIL checks ALL" & popd

echo.
echo === 5. update path, production build (no model READ) ===
"%IV%\iverilog" -g2005 -DREAD_MODEL_EN=0 -DEXPECT_READ_BLOCKED -I tb -o build/update_prod_sim tb/update_tb.v tb/ufm_model.v %RTL% || goto :fail
pushd build & "%IV%\vvp" -n update_prod_sim | findstr /r "^FAIL checks ALL production" & popd

echo.
echo === bus frames for steps 6 and 7 ===
for /f "delims=" %%m in ('py -3.13 -c "import sys;sys.path.insert(0,'python');import paths;print(paths.find_shared('models','rf5_d9_p5_time.joblib'))"') do set "M5=%%m"
for /f "delims=" %%d in ('py -3.13 -c "import sys;sys.path.insert(0,'python');import paths;print(paths.find_shared('data','car_hacking'))"') do set "DATA=%%d"
py -3.13 ..\python\make_sim_frames.py --model "%M5%" --data "%DATA%" --frames 200 --out build/sim_core | findstr Wrote || goto :fail
py -3.13 python\make_swap_frames.py --frames 120 --out build/sim_swap | findstr Wrote || goto :fail
copy /y build\default_model.hex build\sim_core\ >nul
copy /y build\default_model.hex build\sim_swap\ >nul
xcopy /y /e /i /q build\test_blobs build\sim_swap\test_blobs >nul

echo.
echo === 6. the whole design on a bus, IPS on ===
"%IV%\iverilog" -g2005 -I tb -I build/sim_core -o build/core_ips_sim tb/core_frames_tb.v tb/ufm_model.v %RTL% || goto :fail
pushd build\sim_core & "%IV%\vvp" -n ..\core_ips_sim +log=got_ips.csv | findstr /r "booted Played IPS:" & popd
py -3.13 ..\python\compare_sim.py --dir build\sim_core --got got_ips.csv --name compare_ips || goto :fail

echo.
echo === 7. model swap in the middle of traffic ===
"%IV%\iverilog" -g2005 -DSWAP_TEST -I tb -I build/sim_swap -o build/core_swap_sim tb/core_frames_tb.v tb/ufm_model.v %RTL% || goto :fail
pushd build\sim_swap & "%IV%\vvp" -n ..\core_swap_sim +log=got_swap.csv | findstr /r "booted Played IPS: COMMIT" & popd
py -3.13 python\check_swap.py build\sim_swap || goto :fail

echo.
echo All simulations done. Check the lines above for FAIL.
goto :end
:fail
echo.
echo Stopped: the step above failed.
:end
pause
