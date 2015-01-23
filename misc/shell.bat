@echo off

REM Create fake directory pointing to working directory
REM subst h:\ C:\Users\tcfun_000\Documents\workspace\

REM Load Visual Studio commands into path
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64 

REM Put our own executables in the path
set path=h:\handmade\misc;%path%
