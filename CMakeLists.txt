cmake_minimum_required(VERSION 3.26)
project(ThreadedImageProcessor C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -pthread)

add_executable(ThreadedImageProcessor
        McCandlessFilters.c
        BMPHandler.h
        BMPHeader.c
        Image.h
        Image.c
)
target_link_libraries(ThreadedImageProcessor m)


