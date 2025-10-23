cmake_minimum_required(VERSION 3.10)
project(OS_Lab_01 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Опции компиляции
add_compile_options(-Wall -Wextra -Wpedantic)

# Создаем два исполняемых файла: parent и child
add_executable(parent parent.c)
add_executable(child child.c)
