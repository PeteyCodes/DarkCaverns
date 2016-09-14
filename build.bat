@echo off

SET INCLUDE_DIRS=/I"D:\dev\DarkCaverns\sdl2\include"
SET OPTS=/c

SET LIB_DIRS=/LIBPATH:"D:\dev\DarkCaverns\sdl2\lib\x86"
SET LIBS=SDL2.lib SDL2main.lib
SET LINKER_OPTS=/SUBSYSTEM:console


cl %INCLUDE_DIRS% %OPTS% dark.c

link %LIB_DIRS% %LINKER_OPTS% dark.obj %LIBS%
