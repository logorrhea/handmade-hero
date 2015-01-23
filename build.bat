@echo off

mkdir .\build
pushd .\build

cl -Zi ..\src\handmade.cpp

popd
