@echo off

:: =================================================
:: =================================================
:: Edit settings below


:: -----------------
:: Connection Settings
SET "ip=10.42.0.1"
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
:: Output Settings
SET "outdir=data\"
SET "extflags=--binary"







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


if "$is_amp"=="true" "extflags=%extflags% --amplify"


:: add 10% of sample rate to center frequency to prevent DC from overlapping our signal
%py% -c "print(f'{int(%freq% - %srate%*%offset%):g}'" > "%TEMP%\out.txt"
set /p realfreq=<"%TEMP%\out.txt"
del "%TEMP%\out.txt"


md %outdir% 2>nul
%py% -c "import time; print(int(time.time()))" > "%TEMP%\out.txt"
set /p unixtime=<"%TEMP%\out.txt"
set "outfile=%outdir%\data-%unixtime%.bin"
del "%TEMP%\out.txt"


set "runstr=rfsweep measure --ip=%ip% --port=%port% --steps=%steps% --samps=%samps% --stepmode=%stepmode% --file=%outfile% --freq=%realfreq% --band=%band% --srate=%srate% --lna-gain=%lna_gain% --vga-gain=%vga_gain% --samps=%samps% %extflags%"

set "pystr=%py% process.py --freq %freq% %outfile%"

echo %runstr% ^&^& %pystr%
%runstr% && %pystr%

echo.
pause
