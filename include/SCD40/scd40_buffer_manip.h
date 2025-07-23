#ifndef SCD40_BUFFER_MANIP_H
#define SCD40_BUFFER_MANIP_H

#include <stdint.h>
#include "scd40_device_io.h"
#include "../errors.h"

/*******************************************************************************
*                              Defined Constants                               *
*******************************************************************************/

#define SCD40_WORD_SIZE 2
#define SCD40_CRC8_POLYNOMIAL 0x31u
#define SCD40_CRC_LENGTH 1
#define SCD40_HAS_LEADING_ONE(x) ((x & 0x80) == 0x80)

/*******************************************************************************
*                           Function Definitions                               *
*******************************************************************************/

uint8_t scd40_generate_crc(uint8_t* buffer);

int8_t scd40_check_crc(uint8_t* data, uint8_t checksum);

uint32_t scd40_add_uint32_to_buffer(uint8_t* buffer, uint32_t offset, uint32_t data);

uint32_t scd40_add_command_to_buffer(uint8_t* buffer, uint32_t offset, uint16_t data);

int8_t scd40_read_without_crc(uint8_t* buffer, uint16_t expected_size, int* fd);

uint16_t scd40_read_bytes_as_uint16(uint8_t* buffer);

int16_t scd40_read_bytes_as_int16(uint8_t* buffer);

#endif