
set "TMPDIR=tmp\"
set "CMDPATH=lib\cygwin\setup-x86_64.exe"
set "PACKAGES=make,gcc-core,libgcc1"
set "MIRROR=http://cygwin.mirror.constant.com"
:: git,make,gcc-core,binutils,libgcc1,cygwin

:: supress error if directory exists
if not exist "%TMPDIR%" mkdir "%TMPDIR%"

:: download cygwin
:: curl -L -o "%TMPDIR%\setup-x86_64.exe" "https://cygwin.com/setup-x86_64.exe"


cd %TMPDIR%
..\%CMDPATH% -qONndr -R "c:\cygwin64" -s "%MIRROR%" -P "%PACKAGES%
:: -Y -X
cd ..


setx PATH "%PATH%;c:\cygwin64\bin"
