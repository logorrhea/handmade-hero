@echo off

mkdir .\build
pushd .\build

cl -Zi ..\src\handmade.cpp user32.lib

popd
