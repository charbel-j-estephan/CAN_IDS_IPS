@echo off
rem Every simulation of the updatable model. Takes about four hours: the AES
rem circuits make steps 3 and 12 about 45 minutes each and steps 4 and 5 about
rem an hour each. A test that prints FAIL, or no ALL PASS, stops the run.
rem   1. exports the 5, 6 and 8 byte blobs and the test blobs
rem   2. engine latency and throughput
rem   3. every distinct dataset frame vs Python, blob loaded through SPI and COMMIT
rem   4. update path: rejections, power cuts, boot, lockout (development build)
rem   5. the same with READ of the model blocked (production build)
rem   6. the whole design on a simulated bus, IPS on
rem   7. the model swapped from 5 to 8 bytes in the middle of traffic
rem   8. the flash controller against Intel's flash model (needs Questa)
rem   9. the flash controller when the flash stops answering (timeouts)
rem  10. the IPS rate limit
rem  11. the security circuits against Python: AES, the seal (CMAC), the
rem      scrambling keystream (CTR)
rem  12. every dataset frame again, with a scrambled blob over SPI
rem
rem Needs Icarus in C:\iverilog\bin and py -3.13 with scikit-learn and
rem cryptography. The models and the dataset are read from models\ and data\
rem of the repo. The simulations use the public test key (tb\test_key.vh).
setlocal
cd /d "%~dp0"
set "IV=C:\iverilog\bin"
if not exist build mkdir build
set "SEC=rtl/security/aes_sbox.v rtl/security/aes128_enc.v rtl/security/cmac_stream.v rtl/security/ctr_stream.v"
set "RTL=rtl/engine/sdp_ram.v rtl/engine/tree_level.v rtl/engine/rf_engine.v rtl/engine/rf_classifier.v rtl/frontend_can/can_features.v rtl/frontend_can/can_rx.v rtl/frontend_can/ips_flag.v rtl/update/crc32.v rtl/update/spi_slave.v rtl/update/cmd_decoder.v rtl/update/blob_walker.v rtl/update/blob_reader.v rtl/update/update_core.v rtl/nv/nv_ufm_avmm.v rtl/nv/boot_rom.v rtl/top/updatable_core.v %SEC%"

echo === 1. blobs ===
for %%b in (5 6 8) do (
    for /f "delims=" %%m in ('py -3.13 -c "import sys;sys.path.insert(0,'python');import paths;print(paths.find_shared('models','rf5_d9_p%%b_time.joblib'))"') do (
        py -3.13 python/export_blob.py --model "%%m" --out build/blobs/p%%b || goto :fail
    )
)
py -3.13 python/make_test_blobs.py || goto :fail
py -3.13 python/make_security_vectors.py || goto :fail
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
pushd build & "%IV%\vvp" -n update_sim > update_dev.log & popd
findstr /r "^PASS ^FAIL checks ALL" build\update_dev.log
call :passed build\update_dev.log || goto :fail

echo.
echo === 5. update path, production build (no model READ) ===
"%IV%\iverilog" -g2005 -DREAD_MODEL_EN=0 -DEXPECT_READ_BLOCKED -I tb -o build/update_prod_sim tb/update_tb.v tb/ufm_model.v %RTL% || goto :fail
pushd build & "%IV%\vvp" -n update_prod_sim > update_prod.log & popd
findstr /r "^FAIL checks ALL production" build\update_prod.log
call :passed build\update_prod.log || goto :fail

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
echo === 8. flash controller against Intel's flash model (Questa) ===
set "QS=C:\altera_lite\25.1std\quartus"
set "VS=C:\altera_lite\25.1std\questa_fse\win64"
if not exist "%VS%\vsim.exe" (
    echo Questa not found: skipped
    goto :done8
)
if not exist build\intel_flash mkdir build\intel_flash
pushd build\intel_flash
"%QS%\sopc_builder\bin\qsys-generate.exe" ..\..\quartus\nv_flash.qsys --simulation=VERILOG --output-directory=nv_flash_sim >nul
"%VS%\vlib.exe" work >nul
"%VS%\vlog.exe" -quiet -work work "%QS%\eda\sim_lib\mentor\fiftyfivenm_atoms_ncrypt.v" "%QS%\eda\sim_lib\fiftyfivenm_atoms.v" "%QS%\eda\sim_lib\altera_primitives.v" "%QS%\eda\sim_lib\altera_mf.v" "%QS%\eda\sim_lib\220model.v" nv_flash_sim\simulation\submodules\altera_onchip_flash.v nv_flash_sim\simulation\submodules\altera_onchip_flash_avmm_csr_controller.v nv_flash_sim\simulation\submodules\altera_onchip_flash_avmm_data_controller.v nv_flash_sim\simulation\submodules\altera_onchip_flash_util.v nv_flash_sim\simulation\nv_flash.v ..\..\rtl\nv\nv_ufm_avmm.v ..\..\tb\intel_flash_tb.v | findstr /r "Error"
"%VS%\vsim.exe" -c -quiet work.intel_flash_tb -do "run -all; quit -f" | findstr /r "FAIL PASS TIMEOUT"
popd
:done8

echo.
echo === 9. flash controller when the flash stops answering ===
"%IV%\iverilog" -g2005 -o build/nv_timeout_sim tb/nv_timeout_tb.v tb/ufm_model.v rtl/nv/nv_ufm_avmm.v || goto :fail
"%IV%\vvp" -n build/nv_timeout_sim > build\nv_timeout.log
findstr /r "^FAIL checks ALL" build\nv_timeout.log
call :passed build\nv_timeout.log || goto :fail

echo.
echo === 10. IPS rate limit ===
"%IV%\iverilog" -g2005 -o build/ips_limit_sim tb/ips_limit_tb.v rtl/frontend_can/ips_flag.v || goto :fail
"%IV%\vvp" -n build/ips_limit_sim > build\ips_limit.log
findstr /r "^FAIL checks ALL" build\ips_limit.log
call :passed build\ips_limit.log || goto :fail

echo.
echo === 11. security circuits against Python ===
"%IV%\iverilog" -g2005 -o build/aes_sim tb/aes_tb.v rtl/security/aes128_enc.v rtl/security/aes_sbox.v || goto :fail
"%IV%\iverilog" -g2005 -o build/cmac_sim tb/cmac_tb.v %SEC% || goto :fail
"%IV%\iverilog" -g2005 -o build/ctr_sim tb/ctr_tb.v %SEC% || goto :fail
pushd build
"%IV%\vvp" -n aes_sim > aes.log
"%IV%\vvp" -n cmac_sim > cmac.log
"%IV%\vvp" -n ctr_sim > ctr.log
popd
for %%t in (aes cmac ctr) do (
    findstr /r "^AES ^CMAC ^CTR FAIL ALL" build\%%t.log
    call :passed build\%%t.log || goto :fail
)

echo.
echo === 12. every dataset frame, scrambled blob over SPI ===
py -3.13 python/accuracy_engine.py --spi --enc --bytes 5 6 8 || goto :fail

echo.
echo All simulations done, every test said ALL PASS.
goto :end

rem A test's log passes when it has an ALL PASS line and no FAIL line.
:passed
findstr /b /c:"FAIL" "%~1" >nul && exit /b 1
findstr /c:"ALL PASS" "%~1" >nul || exit /b 1
exit /b 0

:fail
echo.
echo Stopped: the step above failed.
:end
pause
