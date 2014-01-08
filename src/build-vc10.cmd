@echo off
echo Compile asmsx with Visual C++ 2010 (10.0)

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio 10.0
set PSDK=C:\Program Files (x86)\Windows Kits\8.1
set INCLUDE=%VCDIR%\VC\INCLUDE;%PSDK%\Include\um
set LIB=%VCDIR%\VC\LIB;%PSDK%\Lib\winv6.3\um\x86
set PATH=%VCDIR%\Common7\IDE;%VCDIR%\VC\BIN;%PATH%

cl.exe test.c > test.vc10.err
rem cl.exe /W4 bdecli.c > bdecli.vc10.err
rem /W1 to /W4 for warnings, /WX to treat warning as errors, /O2 for moderate optimization

if exist *.obj del *.obj
