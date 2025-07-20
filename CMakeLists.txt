cmake_minimum_required(VERSION 3.10)
project(embedded_C VERSION 1.0.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra")

if (CMAKE_BUILD_TYPE STREQUAL "Debug") 
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=undefined")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

if (CMAKE_BUILD_TYPE STREQUAL Test)
    enable_testing()
    include(CTest)
    add_subdirectory(unity)
    add_subdirectory(test)
endif(CMAKE_BUILD_TYPE STREQUAL Test)

add_subdirectory(src)