cmake_minimum_required(VERSION 3.16)
project(tinyunit C)

set(CMAKE_C_STANDARD 99)

option(USE_THREAD_API "Whether to use the thread api" ON)

if(USE_THREAD_API)
    add_definitions(-DTHREAD_API)
endif()

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
