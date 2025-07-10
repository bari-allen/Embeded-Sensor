#ifndef BUFFER_MANIP_H
#define BUFFER_MANIP_H

#include <stdint.h>
#include "device_io.h"

#define WORD_SIZE 2
#define CRC8_POLYNOMIAL 0x31u
#define CRC_LENGTH 1
#define HAS_LEADING_ONE(x) ((x & 0x80) == 0x80)

#define CRCERROR -1
#define NOERROR 0
#define OFFSET_ERROR -3
#define MALLOC_ERR -2

uint8_t generate_crc(uint8_t* buffer);
int8_t check_crc(uint8_t* data, uint8_t checksum);
uint16_t add_uint32_to_buffer(uint8_t* buffer, uint32_t offset, uint32_t data);
uint32_t add_command_to_buffer(uint8_t* buffer, uint32_t offset, uint16_t data);
int8_t read_without_crc(uint8_t* buffer, uint16_t expected_size);
uint16_t read_bytes_as_uint16(uint8_t* buffer);
int16_t read_bytes_as_int16(uint8_t* buffer);
int8_t read_bytes_as_string(uint8_t* buffer, uint16_t expected_size, char* name);

#endif