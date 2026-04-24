@echo off


set def_freq=2.4e9
set def_isamp=n
set def_vga_gain=20
set def_ampl=127


set "extflags="



where python 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python
where python3 1>nul 2>nul
if %ERRORLEVEL%==0 set py=python3
if "%py%"=="" (
    echo Could not find 'python' command. Check if python3 is installed.
    exit /b 1
)


echo.
echo HACKRF TRANSMIT SCRIPT
echo ======================
echo.


set /p "freq=Enter Transmission Frequency (Hz) [%def_freq%]: "
set /p "isamp=Enable Amplifier? (y/n) [%def_isamp%]: "
set /p "vga_gain=Enter TX VGA-Gain (0-47 dB) [%def_vga_gain%]: "
set /p "ampl=Enter Signal Amplitude (0-127) [%def_ampl%]: "



if "%vga_gain%"=="" set "vga_gain=%def_vga_gain%"
if "%freq%"=="" set "freq=%def_freq%"
if "%isamp%"=="" set "isamp=%def_isamp%"
if "%ampl%"=="" set "ampl=%def_ampl%"



%py% -c "print(int(%freq%+0.5))" > "%TEMP%\out.txt"
set /p realfreq=<"%TEMP%\out.txt"
del "%TEMP%\out.txt"


if "%isamp%" neq "y" if "%isamp%" neq "Y" if "%isamp%" neq "yes" if "%isamp%" neq "Yes" goto skipamp
set "extflags=-a 1 %extflags%"
:skipamp



set "transtr=hackrf-tools\hackrf_transfer.exe -c %ampl% -p 1 -R -s 2000000 -f %realfreq% -x %vga_gain% %extflags%"


echo.
echo %transtr%
echo.
echo Transmitting signal at %freq% Hz...
echo Press Ctrl-C or close the window to stop transmission.
echo.


%transtr%

echo.
pause
