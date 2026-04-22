@echo off
setlocal enabledelayedexpansion

set "outdir=data\"
set "extflags=--binary"

set offset=0.1

set def_ip=10.42.0.1
set def_port=12346
set def_freq=2.4e9
set def_srate=10e6
set def_band=
set def_lna_gain=16
set def_vga_gain=20
set def_isamp=n
set def_steps=360
set def_samps=1
set def_smode=1


where python 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python
where python3 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python3
if "%py%"=="" (
    echo Could not find 'python' command. Check if python3 is installed.
    exit /b 1
)


echo.
echo RF ANTENNA MEASURE TOOL
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
set /p "srate=Enter Sample Rate (Hz) [%def_srate%]: "
set /p "band=Enter Band Filter (Hz) [srate*0.75]: "
set /p "_lna_gain=Enter LNA-Gain (0-40 dB) [%def_lna_gain%]: "

if "%_lna_gain%"=="" set "_lna_gain=%def_lna_gain%"

set /a "lna_gain=(_lna_gain+4)/8*8"
if "%_lna_gain%" neq "%lna_gain%" echo Rounding LNA-Gain to %lna_gain% dB.

set /p "_vga_gain=Enter VGA-Gain (0-62 dB) [%def_vga_gain%]: "

if "%_vga_gain%"=="" set "_vga_gain=%def_vga_gain%"

set /a "vga_gain=(_vga_gain+1)/2*2"
if "%_vga_gain%" neq "%vga_gain%" echo Rounding VGA-Gain to %vga_gain% dB.

set /p "isamp=Enable Amplifier? (y/n) [%def_isamp%]: "

echo.
echo Data Options
echo ------------

set /p "steps=Enter Total Sample Steps [%def_steps%]: "
set /p "samps=Enter Samples per Step [%def_samps%]: "
set /p "smode=Enter Microstep Mode (1/2/4/8/16) [%def_smode%]: "

echo.
set /p "dummy=Press Enter to run test..."


if "%isamp%" neq "y" if "%isamp%" neq "Y" if "%isamp%" neq "yes" if "%isamp%" neq "Yes" goto skipamp
set "extflags=%extflags% --amplify"
:skipamp




if "%freq%"=="" set "freq=%def_freq%"
if "%band%"=="" set "band=%def_band%"
if "%isamp%"=="" set "isamp=%def_isamp%"
if "%steps%"=="" set "steps=%def_steps%"
if "%samps%"=="" set "samps=%def_samps%"
if "%smode%"=="" set "smode=%def_smode%"
if "%srate%"=="" set "srate=%def_srate%"



:: if "%band%"=="" (
::     %py% -c "print(int(%srate%*0.75))" > "%TEMP%\out.txt"
::     set /p band=<"%TEMP%\out.txt"
::     del "%TEMP%\out.txt"
:: )


:: offset center frequency by 10% of sample rate to prevent DC from overlapping our signal
:: %py% -c "print(int(%freq% - %srate%*%offset%))" > "%TEMP%\out.txt"
%py% -c "print(f'{int(%freq% - %srate%*%offset%):g}'" > "%TEMP%\out.txt"
set /p realfreq=<"%TEMP%\out.txt"
del "%TEMP%\out.txt"


md %outdir% 2>nul
%py% -c "import time; print(int(time.time()))" > "%TEMP%\out.txt"
set /p unixtime=<"%TEMP%\out.txt"
set "outfile=%outdir%\data-%unixtime%.bin"
del "%TEMP%\out.txt"


set "runstr=rfsweep measure --ip=%ip% --port=%port% --steps=%steps% --samps=%samps% --stepmode=%smode% --file=%outfile% --freq=%realfreq% --band=%band% --srate=%srate% --lna-gain=%lna_gain% --vga-gain=%vga_gain% --samps=%samps% %extflags%"

set "pystr=%py% process.py --freq %freq% %outfile%"

echo.
echo %runstr% ^&^& %pystr%
%runstr% && %pystr%


echo.
pause
