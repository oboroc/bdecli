@echo off
echo Compile with Open Watcom C32 1.9

SET PATH=C:\WATCOM\BINNT;C:\WATCOM\BINW;%PATH%
SET INCLUDE=C:\WATCOM\H;C:\WATCOM\H\NT
SET WATCOM=C:\WATCOM

wcc386.exe bdecli.c
echo.
link386.exe bdecli.obj

if exist *.obj del *.obj
