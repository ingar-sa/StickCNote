@echo off
setlocal

IF NOT EXIST build mkdir build

cd build
win32_main.exe

endlocal
exit
