cmake_minimum_required(VERSION 3.30)
project(Systems C CXX)

set(CMAKE_C_STANDARD 11)

add_executable(7
    7/main.c include/base.h)
add_executable(1 1/main.c include/base.h)
add_executable(2 2/main.c include/base.h)
add_executable(test_gen2 2/test_gen.c)
target_link_libraries(2 m)
add_executable(3 3/main.c include/base.h)
add_executable(meow meow.cpp)
