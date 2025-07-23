#include "../include/functions.h"
#include "scd40_functions.h"

/*******************************************************************************
*                           Function Implementations                           *
*******************************************************************************/

int8_t start_measurement(uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS: 
            return sen55_start_measurement(fd);
        case SCD40_ADDRESS:
            return scd40_start_measurement(fd);
        default:
            return ADDR_ERR;
    }
}

int8_t stop_measurement(uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_stop_measurement(fd);
        case SCD40_ADDRESS:
            return scd40_stop_measurement(fd);
        default:
            return ADDR_ERR;
    }
}

int8_t read_data_flag(bool* is_ready, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_data_flag(is_ready, fd);
        case SCD40_ADDRESS:
            return scd40_read_data_flag(is_ready, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t read_into_buffer(float* data, size_t buffer_size, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_into_buffer(data, buffer_size, fd);
        case SCD40_ADDRESS:
            return scd40_read_into_buffer(data, buffer_size, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t read_product_name(char* name, size_t name_length, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_product_name(name, name_length, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t read_serial_number(char* serial_number, size_t number_length, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_serial_number(serial_number, number_length, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t read_firmware(uint8_t* firmware_version, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_firmware(firmware_version, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t reset(uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_reset(fd);
        default:
            return ADDR_ERR;
    }
}