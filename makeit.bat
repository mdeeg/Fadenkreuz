REM Simple build batch file for Fadenkreuz

REM Paths to installed MinGW tools (e.g. MSYS2, https://www.msys2.org/)
set GCC="C:\msys64\ucrt64\bin\gcc.exe"
set WINDRES="C:\msys64\ucrt64\bin\windres.exe"

%WINDRES% -i fadenkreuz.rc -O coff fadenkreuz.res
%GCC% -fdiagnostics-color=always -municode -s -O3 C:\Users\Matthias\Documents\code\fadenkreuz\fadenkreuz.cpp fadenkreuz.res -mwindows -lstdc++ -lgdi32 -lgdiplus -luser32 -o Fadenkreuz.exe