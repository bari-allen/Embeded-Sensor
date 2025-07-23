#include "../include/buffer_manip.h"
#include "scd40_device_io.h"

/*******************************************************************************
*                           Function Implementations                           *
*******************************************************************************/

uint8_t generate_crc(uint8_t* data, uint8_t device_addr) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_generate_crc(data);
        case SCD40_ADDRESS:
            return scd40_generate_crc(data);
        default:
            return ADDR_ERR;
    }
}

int8_t check_crc(uint8_t* data, uint8_t checksum, uint8_t device_addr) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_check_crc(data, checksum);
        case SCD40_ADDRESS:
            return scd40_check_crc(data, checksum);
        default:
            return ADDR_ERR;
    }
}

uint16_t add_uint32_to_buffer(uint8_t* buffer, uint32_t offset, uint32_t data, uint8_t device_addr) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_add_uint32_to_buffer(buffer, offset, data);
        case SCD40_ADDRESS:
            return scd40_add_uint32_to_buffer(buffer, offset, data);
        default:
            return ADDR_ERR;
    }
}

uint32_t add_command_to_buffer(uint8_t* buffer, uint32_t offset, uint16_t data, uint8_t device_addr) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_add_command_to_buffer(buffer, offset, data);
        case SCD40_ADDRESS:
            return scd40_add_command_to_buffer(buffer, offset, data);
        default:
            return ADDR_ERR;
    }
}

int8_t read_without_crc(uint8_t* buffer, uint16_t expected_size, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_without_crc(buffer, expected_size, fd);
        case SCD40_ADDRESS:
            return scd40_read_without_crc(buffer, expected_size, fd);
        default:
            return ADDR_ERR;
    }
}

uint16_t read_bytes_as_uint16(uint8_t* buffer, uint8_t device_addr) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_bytes_as_uint16(buffer);
        case SCD40_ADDRESS:
            return scd40_read_bytes_as_uint16(buffer);
        default:
            return ADDR_ERR;
    }
}

int16_t read_bytes_as_int16(uint8_t* buffer, uint8_t device_addr) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_bytes_as_int16(buffer);
        case SCD40_ADDRESS:
            return scd40_read_bytes_as_int16(buffer);
        default:
            return ADDR_ERR;
    }
}

int8_t read_bytes_as_string(uint8_t* buffer, uint16_t word_size, char* word, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_read_bytes_as_string(buffer, word_size, word, fd);
        default:
            return ADDR_ERR;
    }
}