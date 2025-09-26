cmake_minimum_required(VERSION 3.26)
project(ThreadedImageProcessor C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -pthread)

add_executable(ThreadedImageProcessor
        Main.c
        headers/BMPHandler.h
        src/BMPHandler.c
        headers/Image.h
        src/Image.c
        headers/filters.h
        headers/macros.h
)
target_link_libraries(ThreadedImageProcessor m)

# ============================================================================
# Build options (user toggles)
# ============================================================================
option(ENABLE_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(ENABLE_ASAN "Enable AddressSanitizer (+LeakSan if available)" ON)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" ON)
option(ENABLE_TSAN "Enable ThreadSanitizer (mutually exclusive with ASAN)" OFF)
option(ENABLE_LTO "Enable Link-Time Optimization (IPO) in Release/RelWithDebInfo" OFF)
option(ENABLE_CLANG_TIDY "Enable clang-tidy if available" OFF)

# ============================================================================
# Helper logic
# ============================================================================
# Warnings
set(PROJECT_WARNING_FLAGS)
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    list(APPEND PROJECT_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
elseif (MSVC)
    list(APPEND PROJECT_WARNING_FLAGS /W4 /permissive-)
endif ()
if (ENABLE_WARNINGS_AS_ERRORS)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        list(APPEND PROJECT_WARNING_FLAGS -Werror)
    elseif (MSVC)
        list(APPEND PROJECT_WARNING_FLAGS /WX)
    endif ()
endif ()

# Sanitizers
if (ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ENABLE_ASAN and ENABLE_TSAN are mutually exclusive.")
endif ()
set(SANITIZER_COMPILE_FLAGS)
set(SANITIZER_LINK_FLAGS)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif ()
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    if (ENABLE_ASAN AND (CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo"))
        list(APPEND SANITIZER_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=address)
    elseif (ENABLE_TSAN AND (CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo"))
        list(APPEND SANITIZER_COMPILE_FLAGS -fsanitize=thread -fno-omit-frame-pointer)
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=thread)
    endif ()
    if (ENABLE_UBSAN AND (CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo"))
        list(APPEND SANITIZER_COMPILE_FLAGS -fsanitize=undefined)
        list(APPEND SANITIZER_LINK_FLAGS -fsanitize=undefined)
    endif ()
endif ()

# LTO (IPO)
if (ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_ok OUTPUT ipo_msg)
    if (ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    else ()
        message(WARNING "IPO/LTO requested but not supported: ${ipo_msg}")
    endif ()
endif ()

# Apply configured flags to the target
if (TARGET ThreadedImageProcessor)
    if (PROJECT_WARNING_FLAGS)
        target_compile_options(ThreadedImageProcessor PRIVATE ${PROJECT_WARNING_FLAGS})
    endif ()
    if (SANITIZER_COMPILE_FLAGS)
        target_compile_options(ThreadedImageProcessor PRIVATE ${SANITIZER_COMPILE_FLAGS})
    endif ()
    if (SANITIZER_LINK_FLAGS)
        target_link_options(ThreadedImageProcessor PRIVATE ${SANITIZER_LINK_FLAGS})
    endif ()
endif ()

# ============================================================================
# Developer summary
# ============================================================================
message(STATUS "Config summary:")
message(STATUS "  Build type              : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Warnings as errors      : ${ENABLE_WARNINGS_AS_ERRORS}")
message(STATUS "  ASan                    : ${ENABLE_ASAN}")
message(STATUS "  UBSan                   : ${ENABLE_UBSAN}")
message(STATUS "  TSan                    : ${ENABLE_TSAN}")
message(STATUS "  LTO/IPO                 : ${ENABLE_LTO}")
message(STATUS "  clang-tidy              : ${ENABLE_CLANG_TIDY}")


