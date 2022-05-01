@echo off
REM This script first sets the environment to compile w vc - vcvarsall x64
REM then compiles. Used when compiling from the game
call "w:/breadboard2/misc/shell.bat"
call "w:/breadboard2/code/build.bat"