# Add the library sources
add_library(buffer_manip_lib buffer_manip.c)
add_library(device_io_lib device_io.c)
add_library(functions_lib functions.c)

# Include headers from the project-wide include/ directory
target_include_directories(buffer_manip_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(device_io_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(functions_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)

# Main executable
add_executable(main main.c)
set_target_properties(main PROPERTIES C_STANDARD 11)
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Link the libraries
target_link_libraries(main
    buffer_manip_lib
    device_io_lib
    functions_lib
)

target_link_libraries(functions_lib PUBLIC buffer_manip_lib)

# Also include headers for the main target
target_include_directories(main PRIVATE ${PROJECT_SOURCE_DIR}/include)