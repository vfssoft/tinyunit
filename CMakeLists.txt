cmake_minimum_required(VERSION 3.16)
project(tinyunit C)

set(CMAKE_C_STANDARD 99)

option(USE_THREAD_API "Whether to use the thread api" ON)

if(USE_THREAD_API)
    add_definitions(-DTHREAD_API)
endif()

add_library(tinyunit_lib STATIC tinyunit.c)

install(
        TARGETS tinyunit_lib
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)

install(
        FILES tinyunit.h
        DESTINATION include
)
