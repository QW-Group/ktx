cmake_minimum_required(VERSION 3.10.0)

project(q3asm C)
add_executable(q3asm q3asm.c q3vm.c cmdlib.c)
