@echo off
setlocal

set ORIGINAL_DIR=%CD%
set SCRIPT_DIR=%~dp0
cd /D %SCRIPT_DIR%
IF NOT EXIST build mkdir build

cd build
del /Q *.pdb > NUL 2> NUL
cd ..

set BuildFolder=build
set FileOutputs=/Fe%BuildFolder%\ /Fo%BuildFolder%\ /Fd%BuildFolder%\
set Libs=user32.lib kernel32.lib gdi32.lib Shell32.lib

set Includes=/I"src"
set CommonCompilerFlags=/MTd /nologo /GL /GR- /EHsc /Od /Oi /W4 /wd4200 /wd4201 /wd4100 /wd4189 /wd4505 /Zi /DUNICODE /std:c++20 %Includes% %FileOutputs%
set CommonLinkerFlags=/Fm%BuildFolder%\ /link %Libs%

cl %CommonCompilerFlags% src/app.cpp /LD %CommonLinkerFlags% /PDB:%BuildFolder%\oms_%random%.pdb /EXPORT:UpdateBackBuffer /EXPORT:RespondToMouseClick 
cl %CommonCompilerFlags% src/win32_main.cpp %CommonLinkerFlags%

cd /D %ORIGINAL_DIR%
endlocal
exit

