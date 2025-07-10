#include "../include/buffer_manip.h"

uint8_t generate_crc(uint8_t* data) {
    uint16_t current_byte;
    uint8_t crc = 0xFF;
    uint8_t bit;

    for (current_byte = 0; current_byte < WORD_SIZE; ++current_byte) {
        crc ^= data[current_byte];
        for (bit = 0; bit < 8; ++bit) {
            if (HAS_LEADING_ONE(crc)) {
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
}

int8_t check_crc(uint8_t* data, uint8_t checksum) {
    if (generate_crc(data) != checksum) {
        return CRCERROR;
    }

    return NOERROR;
}

uint16_t add_uint32_to_buffer(uint8_t* buffer, uint32_t offset, uint32_t data) {
    buffer[offset++] = (uint8_t)((data & 0xFF000000) >> 24);
    buffer[offset++] = (uint8_t)((data & 0x00FF0000) >> 16);
    buffer[offset] = generate_crc(&buffer[offset - WORD_SIZE]);
    ++offset;

    buffer[offset++] = (uint8_t)((data & 0x0000FF00) >> 8);
    buffer[offset++] = (uint8_t)((data & 0x000000FF));
    buffer[offset] = generate_crc(&buffer[offset - WORD_SIZE]);
    ++offset;

    return offset;
}

uint32_t add_command_to_buffer(uint8_t* buffer, uint32_t offset, uint16_t data) {
    buffer[offset++] = (uint8_t)((data & 0xFF00) >> 8);
    buffer[offset++] = (uint8_t)((data & 0x00FF));

    return offset;
}

int8_t read_without_crc(uint8_t* buffer, uint16_t expected_size) {
    int error;
    uint32_t i, j;
    uint16_t size = (expected_size / WORD_SIZE) * (WORD_SIZE + CRC_LENGTH);

    if (expected_size % WORD_SIZE != 0) {
        return OFFSET_ERROR;
    }

    error = device_read(buffer, size);
    if (error != 0) {
        return error;
    }

    for (i = 0, j = 0; i < size; i += WORD_SIZE + CRC_LENGTH) {
        error = check_crc(&buffer[i], buffer[i + WORD_SIZE]);
        if (error != 0) {
            return error;
        }

        buffer[j++] = buffer[i];
        buffer[j++] = buffer[i + 1];
    }

    return NOERROR;
}

uint16_t read_bytes_as_uint16(uint8_t* buffer) {
    uint16_t MSB = buffer[0] << 8;
    uint16_t LSB = buffer[1];
    
    return MSB | LSB;
}

int16_t read_bytes_as_int16(uint8_t* buffer) {
    uint16_t MSB = buffer[0] << 8;
    uint16_t LSB = buffer[1];

    return (int16_t)(MSB | LSB);
}

int8_t read_bytes_as_string(uint8_t* buffer, uint16_t expected_size, char* name) {
    int8_t error;

    error = read_without_crc(buffer, expected_size);
    if (error != 0) {
        return error;
    }

    for (int i = 0; i < expected_size; ++i) {
        name[i] = buffer[i];
        
        if (buffer[i] == '\0') {break;}
    }

    return NOERROR;
}