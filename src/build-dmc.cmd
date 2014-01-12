@echo off
echo Compile asmsx with Digital Mars Compiler

SET PATH=C:\dm\bin;%PATH%
SET INCLUDE=C:\dm\include;..\..\compat_s\src
set LIB=C:\dm\lib

dmc.exe bdecli.c

if exist *.obj del *.obj
if exist *.map del *.map
