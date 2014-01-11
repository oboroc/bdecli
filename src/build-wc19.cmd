@echo off
echo Compile asmsx with Open Watcom C32 1.9

SET PATH=C:\WATCOM\BINW;C:\WATCOM\BINNT;%PATH%
SET INCLUDE=C:\WATCOM\H;C:\WATCOM\H\NT;..\..\compat_s\src
SET WATCOM=C:\WATCOM

wcc386.exe bdecli.c
echo.
link386.exe bdecli.obj

if exist *.obj del *.obj
