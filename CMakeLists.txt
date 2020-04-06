cmake_minimum_required(VERSION 3.8)
project(postlinker)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c utils.c utils.h postlinker.c postlinker.h parser.c parser.h)
add_executable(postlinker ${SOURCE_FILES})