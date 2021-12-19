cmake_minimum_required( VERSION 3.4.0 )
# Если версия установленой программы
# старее указаной, произайдёт аварийный выход.

project(q3asm)
add_executable(q3asm q3asm.c q3vm.c cmdlib.c)       # Создает исполняемый файл с именем main
# из исходника main.cpp
