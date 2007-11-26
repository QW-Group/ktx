
@echo off

rem $Id$

rem NOTE: u must look %SRC%\game.q3asm so u can guess where qwprogs.qvm locate

set CC=lcc
set Q3ASM=q3asm
set CFLAGS=-DQ3_VM -S -Wf-target=bytecode -Wf-g
set INCFLAGS=-I..\include
set SRC=..\src

rem make sure we have a safe environment
set LIBRARY=
set INCLUDE=

mkdir VM
cd VM

%CC% %CFLAGS% %INCFLAGS% %SRC%\bg_lib.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\buttons.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\client.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\combat.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\doors.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\g_main.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\g_cmd.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\g_userinfo.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\g_mem.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\g_spawn.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\g_utils.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\items.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\mathlib.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\misc.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\plats.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\player.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\q_shared.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\server.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\spectate.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\subs.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\triggers.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\weapons.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\world.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\globals.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\admin.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\captain.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\commands.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\logs.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\maps.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\match.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\motd.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\vip.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\vote.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\ctf.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\runes.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\grapple.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\arena.c
@if errorlevel 1 goto quit


copy %SRC%\g_syscalls.asm .


@if errorlevel 1 goto quit

echo q3asm.

%Q3ASM% -f %SRC%\game

:quit

cd ..

