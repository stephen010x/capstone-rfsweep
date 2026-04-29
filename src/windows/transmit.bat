@echo off
setlocal enabledelayedexpansion


set def_ip=10.42.0.1
set def_port=12346
set def_freq=2.4e9
set def_vga_gain=20
set def_isamp=y
set def_clock=y
set def_ampl=127


where python 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python
where python3 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python3
if "%py%"=="" (
    echo Could not find 'python' command. Check if python3 is installed.
    exit /b 1
)


echo.
echo RF ANTENNA TRANSMIT TOOL
echo =======================
echo.
echo For the following options, enter a value and press enter.
echo (leave blank and hit enter for default value)


echo.
echo Connection Options
echo ------------------

set /p "ip=Enter Controller IP [%def_ip%]: "
set /p "port=Enter Controller Port [%def_port%]: "

if "%ip%"=="" set ip=%def_ip%
if "%port%"=="" set port=%def_port%

rfsweep ping --ip=%ip% --port=%port% 1>nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo.
    echo Unable to connect to Controller on ^<%ip%:%port%^>.
    echo Check if correct IP and Port, and check if Controller is on.
    exit /b 1
)

echo.
echo HackRF Options
echo --------------

set /p "freq=Enter Center Freq (Hz) [%def_freq%]: "
set /p "isamp=Enable Amplifier? (y/n) [%def_isamp%]: "
set /p "vga_gain=Enter VGA-Gain (0-47 dB) [%def_vga_gain%]: "
set /p "ampl=Enter Transmitting Amplitude (0-127) [%def_ampl%]: "
set /p "clock=Enable Transmitter Clock Out? (y/n) [%def_clock%]: "

echo.
set /p "dummy=Press Enter to run test..."



if "%freq%"==""  set "freq=%def_freq%"
if "%ampl%"==""  set "ampl=%def_ampl%"
if "%isamp%"=="" set "isamp=%def_isamp%"
if "%clock%"=="" set "ampl=%def_clock%"
if "%vga_gain%"=="" set "vga_gain=%def_vga_gain%"


if "%isamp:~0,1%" neq "y" if "%isamp:~0,1%" neq "Y" goto skipamp
set "extflags=%extflags% --amplify"
:skipamp

if "%clock:~0,1%" neq "y" if "%clock:~0,1%" neq "Y" goto skipclock
set "extflags=%extflags% --clock"
:skipclock




set "txstartstr=rfsweep transmit enable --ip=%ip% --port=%port% --freq=%freq% --vga-gain=%vga_gain% --tx-ampl=%ampl% %extflags%"

set "txendstr=rfsweep transmit disable --ip=%ip% --port=%port%"


echo.

%startstr%
echo.




echo.
echo %txstartstr%
%txstartstr% || (
    :: I hate powershell so much.
    echo.
    echo === SERVER LOGS ===
    echo -------------------
    %errstr% | powershell -noprofile -command ^
        "Get-Content -Raw - | Select-Object -Last 10"
)
echo.
echo Press Enter or close window to stop transmitter...
pause >nul
echo %txendstr%
%txendstr%


