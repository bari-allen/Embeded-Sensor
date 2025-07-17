cmake_minimum_required(VERSION 3.10)
project(embedded_C VERSION 1.0.0 LANGUAGES C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -Wall -Wextra")

add_subdirectory(src)