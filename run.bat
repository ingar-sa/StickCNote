@echo off
setlocal

IF NOT EXIST build mkdir build

cd build
remedybg StickCNote.exe

endlocal
exit
