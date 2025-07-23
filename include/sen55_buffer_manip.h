#ifndef SEN55_BUFFER_MANIP_H
#define SEN55_BUFFER_MANIP_H

#include <stdint.h>
#include "sen55_device_io.h"
#include "errors.h"

/*******************************************************************************
*                              Defined Constants                               *
*******************************************************************************/

#define WORD_SIZE 2
#define CRC8_POLYNOMIAL 0x31u
#define CRC_LENGTH 1
#define HAS_LEADING_ONE(x) ((x & 0x80) == 0x80)

/*******************************************************************************
*                           Function Definitions                               *
*******************************************************************************/

/**
 * @brief Generates the CRC from the first two item in the buffer
 * 
 * Uses the CRC-8 algorithm to generate the checksum from the first two elements
 * in the buffer
 * 
 * @param buffer array of bytes 
 * @return the checksum from the first two bytes in the buffer
 */
uint8_t sen55_generate_crc(uint8_t* buffer);

/**
 * @brief Compares the checksum generated against the inputted checksum
 * 
 * Generates the actual checksum using the generate_crc() function and compares the
 * the output against the inputted checksum
 * 
 * @param data array of bytes
 * @param checksum the checksum returned by the I2C device
 * @return CRCERROR if the checksums don't match and NOERROR if they do
 */
int8_t sen55_check_crc(uint8_t* data, uint8_t checksum);

/**
 * @brief Adds the inputted uint32_t to the buffer 2 bytes
 * 
 * Adds the inputted uint32_t to the buffer 2 bytes at a time then adds their checksum
 * after each 2 bytes
 * 
 * @param buffer buffer to write the uint32_t
 * @param offset the number of elements already in the buffer
 * @param data the uint32_t to be written to the buffer
 * @return the offset with the uint32_t added
 */
uint16_t sen55_add_uint32_to_buffer(uint8_t* buffer, uint32_t offset, uint32_t data);

/**
 * @brief Adds the given address pointer to the buffer
 * 
 * Adds the address pointer 2 bytes at a time without a checksum after each 2 bytes
 * 
 * @param buffer the buffer to write the address pointer 
 * @param offset the number of elements already in the buffer
 * @param data the address pointer to add to the buffer
 * @return the offset with the address pointer added
 */
uint32_t sen55_add_command_to_buffer(uint8_t* buffer, uint32_t offset, uint16_t data);

/**
 * @brief Reads the data from the I2C device and removes the checksum after each 2 bytes
 * 
 * Removes the checksum after each 2 bytes only if the checksum matches the generated checksum
 * The expected number of bytes must be a multiple of 2 or else an OFFSET_ERROR is returned
 * 
 * @param buffer the buffer to write the data from the I2C device
 * @param expected_size the expected number of bytes in the buffer without the checksum
 * @return an error if the data couldn't be written, read, or the offset is wrong, else 
            NOERROR is returned
 */
int8_t sen55_read_without_crc(uint8_t* buffer, uint16_t expected_size, int* fd);

/**
 * @brief Takes the first two bytes and generates a uint16_t from them
 * 
 * Shifts the first element to the left by 8 bits and ORs it with the second element
 * 
 * @param buffer a buffer of uint8_t's 
 * @return the first 2 bytes as a uint16_t
 */
uint16_t sen55_read_bytes_as_uint16(uint8_t* buffer);

/**
 * @brief Takes the first two bytes and generates an int16_t from them
 * 
 * Shifts the first element to the left by 8 bits, ORs it with the second element, 
 * then casts it to an int16_t
 * 
 * @param buffer a buffer of uint8_t's
 * @return the first 2 bytes as an int16_t
 */
int16_t sen55_read_bytes_as_int16(uint8_t* buffer);

/**
 * @brief Reads the bytes up to the null terminating character ('\0') as chars
 * 
 * Reads the bytes up to the null terminating character as chars and adds them to the name
 * out parameter
 * 
 * @param buffer a buffer of uint8_t's interpreted as chars
 * @param word_size the expected number of characters
 * @param word the name out parameter
 * @return an error if the data couldn't be read from the device, else NOERROR is returned
 */
int8_t sen55_read_bytes_as_string(uint8_t* buffer, uint16_t word_size, char* word, int* fd);


#endif