cmake_minimum_required(VERSION 3.30)
project(Systems C)

set(CMAKE_C_STANDARD 11)

add_executable(7
    7/main.c include/base.h)
add_executable(1 1/main.c include/base.h)
add_executable(2 2/main.c include/base.h)
