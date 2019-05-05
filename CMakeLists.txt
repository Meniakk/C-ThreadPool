cmake_minimum_required(VERSION 3.14)
project(Ass4OSC C)

set(CMAKE_C_STANDARD 99)
SET(CMAKE_C_FLAGS -pthread)

add_executable(Ass4OSC main.c osqueue.h osqueue.c threadPool.h threadPool.c test.c)