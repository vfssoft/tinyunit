cmake_minimum_required(VERSION 3.16)
project(tinyunit C)

set(CMAKE_C_STANDARD 99)

add_library(tinyunit tinyunit.c)

install(
        TARGETS tinyunit
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

install(
        FILES tinyunit.h
        DESTINATION include
)
