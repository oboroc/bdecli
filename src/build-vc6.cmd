@echo off
echo Compile asmsx with Visual C++ 98 (6.0)

set VCDIR=C:\Program Files\Microsoft Visual Studio
set INCLUDE=%VCDIR%\VC98\INCLUDE
set LIB=%VCDIR%\VC98\LIB
set PATH=%VCDIR%\Common\msdev98\bin;%VCDIR%\VC98\bin;%PATH%

cl.exe /G6 /W3 bdecli.c > bdecli.vc6.err
rem /W1 to /W4 for warnings, /WX to treat warning as errors, /O2 for moderate optimization

del *.obj
