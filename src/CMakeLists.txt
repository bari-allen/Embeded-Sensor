set(CMAKE_PREFIX_PATH "/usr/local")
find_package(eclipse-paho-mqtt-c REQUIRED)
find_package(cJSON REQUIRED)

add_library(sen55_device_io_lib ./SEN55/sen55_device_io.c)
add_library(sen55_functions_lib ./SEN55/sen55_functions.c)
add_library(sen55_buffer_manip_lib ./SEN55/sen55_buffer_manip.c)

add_library(scd40_device_io_lib ./SCD40/scd40_device_io.c)
add_library(scd40_functions_lib ./SCD40/scd40_functions.c)
add_library(scd40_buffer_manip_lib ./SCD40/scd40_buffer_manip.c)

# Add the library sources
add_library(buffer_manip_lib buffer_manip.c)
add_library(device_io_lib device_io.c)
add_library(functions_lib functions.c)

# Include headers from the project-wide include/ directory
target_include_directories(sen55_device_io_lib PUBLIC ${PROJECT_SOURCE_DIR}/include/SEN55)
target_include_directories(sen55_functions_lib PUBLIC ${PROJECT_SOURCE_DIR}/include/SEN55)
target_include_directories(sen55_buffer_manip_lib PUBLIC ${PROJECT_SOURCE_DIR}/include/SEN55)

target_include_directories(scd40_device_io_lib PUBLIC ${PROJECT_SOURCE_DIR}/include/SCD40)
target_include_directories(scd40_functions_lib PUBLIC ${PROJECT_SOURCE_DIR}/include/SCD40)
target_include_directories(scd40_buffer_manip_lib PUBLIC ${PROJECT_SOURCE_DIR}/include/SCD40)

target_include_directories(buffer_manip_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(device_io_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_include_directories(functions_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(sen55_buffer_manip_lib PUBLIC sen55_device_io_lib)
target_link_libraries(sen55_functions_lib PUBLIC sen55_buffer_manip_lib sen55_device_io_lib)

target_link_libraries(scd40_buffer_manip_lib PUBLIC scd40_device_io_lib)
target_link_libraries(scd40_functions_lib PUBLIC scd40_buffer_manip_lib scd40_device_io_lib)

target_link_libraries(buffer_manip_lib PUBLIC sen55_buffer_manip_lib scd40_buffer_manip_lib)
target_link_libraries(device_io_lib PUBLIC sen55_device_io_lib scd40_device_io_lib)
target_link_libraries(functions_lib PUBLIC sen55_functions_lib scd40_functions_lib)

add_executable(publisher publisher.c)
set_target_properties(publisher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

target_link_libraries(publisher
    buffer_manip_lib
    device_io_lib
    functions_lib
    eclipse-paho-mqtt-c::paho-mqtt3c
    cjson
    )