#!/bin/bash

CC=lcc
Q3ASM=q3asm
CFLAGS=-DQ3_VM -S -Wf-target=bytecode -Wf-g
INCFLAGS=-I../include
SRC=../src

LIBRARY=
INCLUDE=

mkdir VM
cd VM
for filename in ../src/*.c; do
  lcc -DQ3_VM -S -Wf-target=bytecode -Wf-g -I../include "$filename"
done
cp -f ../src/g_syscalls.asm .
q3asm -f ../src/game


