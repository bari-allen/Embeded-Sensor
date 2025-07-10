#include <stdint.h>

#define WORD_SIZE 2
#define CRC8_POLYNOMIAL 0x31u
#define HAS_LEADING_ONE(x) ((x & 0x80))

uint8_t generate_crc(uint8_t* data) {
    uint16_t current_byte;
    uint8_t crc = 0xFF;
    uint8_t bit;

    for (current_byte = 0; current_byte < WORD_SIZE; ++current_byte) {
        crc ^= data[current_byte];
        for (bit = 0; bit < 8; ++bit) {
            if (HAS_LEADING_ONE(crc)) {
                crc = (crc << 1) & CRC8_POLYNOMIAL;
            } else {
                crc = (crc << 1);
            }
        }
    }

    return crc;
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