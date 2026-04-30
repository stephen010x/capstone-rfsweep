@echo off

:: =================================================
:: =================================================
:: Edit settings below. (avoid whitespace)


:: -----------------
:: Connection Settings
:: It will try all three of these IPs
SET "ipA=10.42.0.1" & REM - Wifi IP
SET "ipB=10.42.0.1" & REM - Ethernet IP
SET "ipC=127.0.0.1" & REM - Localhost IP
SET "port=12346"

:: -----------------
:: Hackrf Settings
SET "freq=2.4e9"    & REM - center frequency
SET "srate=10e6"    & REM - sample rate (Hz)
SET "band="         & REM - bandpass filter width. Leave blank to set to 75% of srate
SET "lna_gain=16"   & REM - LNA Gain (amplifies small signals) (0-40 dB, step of 8 dB)
SET "vga_gain=20"   & REM - VGA Gain (software amplifier) (0-62 dB, step of 2 dB)
SET "is_amp=false"  & REM - enable amplifier
SET "steps=360"     & REM - how many angles to take samples
SET "samps=1"       & REM - how many samples to take at each angle
SET "stepmode=1"    & REM - microstep mode (1/2/4/8/16)

:: -----------------
:: Transmitter Settings
SET "tx_enable=true"    & REM - enable transmitter?
SET "tx_vga_gain=20"    & REM - TX VGA Gain (0-47 dB, steps of 1 dB)
SET "tx_ampl=127"       & REM - TX signal amplitude (0-127)
SET "tx_is_amp=true"    & REM - enable TX amplifier?
SET "tx_clock=true"     & REM - enable TX clock out?

:: -----------------
:: Output Settings
SET "outdir=data\"      & REM - output directory
SET "extflags=--binary" & REM - extra flags







:: =================================================
:: =================================================
:: You don't need to edit anything past this point







set "offset=0.1"    & REM - offsets our center frequency by offset*srate


where python 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python
where python3 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python3
if "%py%"=="" (
    echo "Could not find 'python' command. Check if python3 is installed."
    exit /b 1
)


if "%is_amp%"=="true"    set "extflags=%extflags% --amplify"
if "%tx_is_amp%"=="true" set "tx_extflags=%tx_extflags% --amplify"
if "%tx_clock%"=="true"  set "tx_extflags=%tx_extflags% --clock"


:: add 10% of sample rate to center frequency to prevent DC from overlapping our signal
%py% -c "print(f'{int(%freq% - %srate%*%offset%):g}')" > "%TEMP%\out.txt"
set /p realfreq=<"%TEMP%\out.txt"
del "%TEMP%\out.txt"


md %outdir% 2>nul
%py% -c "import time; print(int(time.time()))" > "%TEMP%\out.txt"
set /p unixtime=<"%TEMP%\out.txt"
set "outfile=%outdir%\data-%unixtime%.bin"
del "%TEMP%\out.txt"




:: select working IP
((
((rfsweep ping --ip=%ipA% --port=%port%) && set ip=%ipA%) || ^
((rfsweep ping --ip=%ipB% --port=%port%) && set ip=%ipB%) || ^
((rfsweep ping --ip=%ipC% --port=%port%) && set ip=%ipC%)
) 1>NUL 2>NUL) || (
    echo.
    echo Tried ^<%ipA%:%port%^>, ^<%ipB%:%port%^>, ^<%ipC%:%port%^>
    echo Unable to connect to server. Check if IP address is correct, or device isn't turned off.
    exit /b 1
)



set "runstr=rfsweep measure --ip=%ip% --port=%port% --steps=%steps% --samps=%samps% --stepmode=%stepmode% --file=%outfile% --freq=%realfreq% --band=%band% --srate=%srate% --lna-gain=%lna_gain% --vga-gain=%vga_gain% --samps=%samps% %extflags%"

set "txstartstr=rfsweep transmit enable --ip=%ip% --port=%port% --freq=%freq% --vga-gain=%tx_vga_gain% --tx-ampl=%tx_ampl% %tx_extflags%"

set "txendstr=rfsweep transmit disable --ip=%ip% --port=%port%"

set "errstr=rfsweep getlogs --ip=%ip% --port=%port%"

set "pystr=%py% process.py --freq %freq% %outfile%"



echo.

:: start transmitter
if "%tx_enable%"=="true" (
    echo %txstartstr%
    %txstartstr% || (
        :: I hate powershell so much.
        echo.
        echo === SERVER LOGS ===
        echo -------------------
        %errstr% | powershell -noprofile -command ^
            "Get-Content -Raw - | Select-Object -Last 10"
        pause
        exit /b 1
    )
)

:: take measurements
echo.
echo %runstr% ^&^& %pystr%
(
    %runstr% || (
        :: I hate powershell so much.
        echo.
        echo === SERVER LOGS ===
        echo -------------------
        %errstr% | powershell -noprofile -command ^
            "Get-Content -Raw - | Select-Object -Last 10"
        cmd /c "exit /b 5"
    )
) && %pystr%


:: stop transmitter
if "%tx_enable%"=="true" (
    echo.
    echo Press Enter or close window to stop transmitter...
    pause >nul
    echo %txendstr%
    %txendstr%

    
) else (
    echo.
    pause
)

