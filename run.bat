@echo off
setlocal

IF NOT EXIST build mkdir build

cd build
main.exe

endlocal
exit
