cmake_minimum_required(VERSION 3.26)
project(ThreadedImageProcessor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -pthread)

add_executable(ThreadedImageProcessor
        Main.c
        headers/BMPHandler.h
        src/BMPHeader.c
        headers/Image.h
        src/Image.c
        headers/filters.h
)
target_link_libraries(ThreadedImageProcessor m)


