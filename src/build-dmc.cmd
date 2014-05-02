@echo off
echo Compile asmsx with Digital Mars Compiler

SET PATH=C:\dm\bin;%PATH%
SET INCLUDE=C:\dm\include
set LIB=C:\dm\lib

dmc.exe -A -r -w- -o+space -6 bdecli.c

if exist *.obj del *.obj
if exist *.map del *.map
