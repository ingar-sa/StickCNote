@echo off
setlocal

IF NOT EXIST build mkdir build

cd build
StickCNote.exe

endlocal
exit
