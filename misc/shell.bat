@echo off

REM Load Visual Studio commands into path
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64 

REM Put our own executables in the path
set path=c:\workspace\handmade-hero\misc;%path%
