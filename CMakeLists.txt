cmake_minimum_required(VERSION 3.9)
project(cbot C)

set(CMAKE_C_STANDARD 99)

add_executable(cbot main.c socket.h socket.c io.h io.c)