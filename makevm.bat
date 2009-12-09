@echo off

rem $Id$

rem NOTE: you must look %SRC%\game.q3asm so u can guess where qwprogs.qvm locate

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
%CC% %CFLAGS% %INCFLAGS% %SRC%\clan_arena.c
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
%CC% %CFLAGS% %INCFLAGS% %SRC%\race.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_ai.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_boss.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_client.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_demon.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_dog.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_enforcer.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_fish.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_hknight.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_knight.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_monsters.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_ogre.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_oldone.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_shalrath.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_shambler.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_soldier.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_tarbaby.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_wizard.c
@if errorlevel 1 goto quit
%CC% %CFLAGS% %INCFLAGS% %SRC%\sp_zombie.c
@if errorlevel 1 goto quit


copy %SRC%\g_syscalls.asm .


@if errorlevel 1 goto quit

echo q3asm.

%Q3ASM% -f %SRC%\game

:quit

cd ..

