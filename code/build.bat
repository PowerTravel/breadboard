@echo off

REM  -wd4505 turns off warnings that a function is not referenced
REM  -wd4244 turns off warning about truncation loss of data when converting from real to int
..\ctime\ctime -begin handmade_hero.ctm

REM /Od removes compiler optimization
REM /O2 Compiles in optimized mode

REM -wd4018 Disables Waring: < Signed/Unsigned mismatch

set CommonCompilerFlags= /DEBUG:FULL -Od -nologo -fp:fast -fp:except-     -GR- -EHa- -Zo -Oi -WX -W4 -wd4018 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4706 -FC -Z7 -GS- -Gs9999999 -wd4702
set CommonCompilerFlags=-DHANDMADE_PROFILE=1 -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -DUSING_OPENGL=1 -DHANDMADE_WIN32=1 %CommonCompilerFlags%

set CommonLinkerFlags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 32-bit build
REM cl %CommonCompilerFlags% ..\handmade\code\win32_handmade.cpp  /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM 64-bit build
REM Optimization switches -O2
del *.pdb > NUL 2> NUL
REM echo create lock file
echo WAITING FOR PDB > lock.tmp
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=0 ..\code\breadboard.cpp -Fmhandmade.map -MTd -LD /link -incremental:no -opt:ref  %CommonLinkerFlags% -PDB:handmade_%random%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GameGetSoundSamples -EXPORT:DEBUGGameFrameEnd
set LastError=%ERRORLEVEL%
del lock.tmp
cl %CommonCompilerFlags% -DTRANSLATION_UNIT_INDEX=1 ..\code\win32_breadboard.cpp -Fmwin32_handmade.map /link %CommonLinkerFlags%


rem  gdi32.lib winmm.lib opengl32.lib
rem cl /Zi -nologo  ..\handmade\code\win32_opengl_test.cpp  /link -incremental:no user32.lib gdi32.lib opengl32.lib
popd


..\ctime\ctime -end handmade_hero.ctm %LastError%