@echo off
setlocal

set ORIGINAL_DIR=%CD%
set SCRIPT_DIR=%~dp0
cd /D %SCRIPT_DIR%
cd ..
IF NOT EXIST build mkdir build

cd build
main.exe

cd /D %ORIGINAL_DIR%
endlocal
exit
