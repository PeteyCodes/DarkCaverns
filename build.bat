@echo off

del dark.exe
del *.pdb

SET INCLUDE_DIRS=/I"D:\dev\DarkCaverns\sdl2\include"
SET OPTS=/c
SET DEBUG=/DDEBUG /Zi /Fddark.pdb

SET LIB_DIRS=/LIBPATH:"D:\dev\DarkCaverns\sdl2\lib\x86"
SET LIBS=SDL2.lib SDL2main.lib
SET LINKER_OPTS=/SUBSYSTEM:console /DEBUG

cl %INCLUDE_DIRS% %OPTS% %DEBUG% dark.c

link %LIB_DIRS% %LINKER_OPTS% dark.obj %LIBS%

del *.obj