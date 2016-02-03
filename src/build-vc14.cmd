@echo off
echo Compile with Visual C++ 2015 (14.0) and Windows 10 SDK

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 14.0
set PSDK=C:\Program Files (x86)\Windows Kits\10
set INCLUDE=%VCDIR%\VC\INCLUDE;%PSDK%\Include\10.0.10586.0\um;%PSDK%\Include\10.0.10586.0\ucrt
set LIB=%VCDIR%\VC\LIB;%PSDK%\Lib\10.0.10586.0\um\x86;%PSDK%\Lib\10.0.10586.0\ucrt\x86
set PATH=%VCDIR%\Common7\IDE;%VCDIR%\VC\BIN;%PATH%

cl.exe /nologo /TC /W4 /O2 bdecli.c > bdecli.vc14.err
rem /W1 to /W4 for warnings, /WX to treat warning as errors, /O2 for moderate optimization

if exist *.obj del *.obj
