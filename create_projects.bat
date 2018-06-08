@echo off

echo creating projects...

premake5 vs2017
premake5 export-compile-commands
premake5 cmake

echo finished.