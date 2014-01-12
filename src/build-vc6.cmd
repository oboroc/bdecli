@echo off
echo Compile asmsx with Visual C++ 98 (6.0)

set VCDIR=C:\Program Files\Microsoft Visual Studio
set INCLUDE=%VCDIR%\VC98\INCLUDE;..\..\compat_s\src
set LIB=%VCDIR%\VC98\LIB
set PATH=%VCDIR%\Common\msdev98\bin;%VCDIR%\VC98\bin;%PATH%

cl.exe /nologo /G6 /W4 bdecli.c > bdecli.vc6.err
rem /W1 to /W4 for warnings, /WX to treat warning as errors, /O2 for moderate optimization

if exist *.obj del *.obj *.i
