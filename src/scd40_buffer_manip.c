#include "../include/scd40_buffer_manip.h"
#include "errors.h"

/*******************************************************************************
*                           Function Implementations                           *
*******************************************************************************/

uint8_t scd40_generate_crc(uint8_t* data) {
    uint16_t current_byte;
    uint8_t crc = 0xFF;
    uint8_t bit;

    for (current_byte = 0; current_byte < SCD40_WORD_SIZE; ++current_byte) {
        crc ^= data[current_byte];
        for (bit = 0; bit < 8; ++bit) {
            if (SCD40_HAS_LEADING_ONE(crc)) {
                crc = (crc << 1) ^ SCD40_CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

int8_t scd40_check_crc(uint8_t* data, uint8_t checksum) {
    return (scd40_generate_crc(data) != checksum) ? CRC_ERR : NOERR;
}

uint32_t scd40_add_uint32_to_buffer(uint8_t *buffer, uint32_t offset, uint32_t data) {
    buffer[offset++] = (uint8_t)((data & 0xFF000000) >> 24);
    buffer[offset++] = (uint8_t)((data & 0x00FF0000) >> 16);
    buffer[offset] = scd40_generate_crc(&buffer[offset - SCD40_WORD_SIZE]);
    ++offset;

    buffer[offset++] = (uint8_t)((data & (0x0000FF00)) >> 8);
    buffer[offset++] = (uint8_t)((data & 0x000000FF));
    buffer[offset] = scd40_generate_crc(&buffer[offset - SCD40_WORD_SIZE]);
    ++offset;

    return offset;
}

uint32_t scd40_add_command_to_buffer(uint8_t *buffer, uint32_t offset, uint16_t data) {
    buffer[offset++] = (uint8_t)((data & 0xFF00) >> 8);
    buffer[offset++] = (uint8_t)((data & 0x00FF));

    return offset;
}

int8_t scd40_read_without_crc(uint8_t *buffer, uint16_t expected_size, int* fd) {
    int error;
    uint32_t i, j;
    uint16_t size = (expected_size / SCD40_WORD_SIZE) * (SCD40_WORD_SIZE + SCD40_CRC_LENGTH);

    if (expected_size % SCD40_WORD_SIZE != 0) {
        return OFFSET_ERR;
    }

    if ((error = scd40_device_read(buffer, size, fd)) != NOERR) {
        return error;
    }

    for (i = 0, j = 0; i < size; i += SCD40_WORD_SIZE + SCD40_CRC_LENGTH) {
        if ((error = scd40_check_crc(&buffer[i], buffer[i + SCD40_WORD_SIZE])) != NOERR) {
            return error;
        }

        buffer[j++] = buffer[i];
        buffer[j++] = buffer[i + 1];
    }

    return NOERR;
}

uint16_t scd40_read_bytes_as_uint16(uint8_t *buffer) {
    uint16_t MSB = buffer[0] << 8;
    uint16_t LSB = buffer[1];

    return MSB | LSB;
}

int16_t scd40_read_bytes_as_int16(uint8_t *buffer) {
    return (int16_t)scd40_read_bytes_as_uint16(buffer);
}