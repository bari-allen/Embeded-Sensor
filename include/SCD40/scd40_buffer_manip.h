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

/**
 * @brief Generates the CRC for the first two elements in the inputted buffer
 * 
 * @param buffer the data to be written to the device
 * @return the CRC for the first two elements in the buffer 
 */
uint8_t scd40_generate_crc(uint8_t* buffer);

/**
 * @brief Compares the checksum recieved from the device against the generated checksum
 * 
 * @param data the data read from the device
 * @param checksum the checksum recieved from the device
 * @return CRC_ERR if the checksums don't match, NOERR otherwise
 */
int8_t scd40_check_crc(uint8_t* data, uint8_t checksum);

/**
 * @brief Adds a uint32_t number to the buffer
 * 
 * @param buffer the data to be written to the device
 * @param offset the number of elements already in the buffer
 * @param data the uint32_t number to be added
 * @return the new offset of the buffer
 */
uint32_t scd40_add_uint32_to_buffer(uint8_t* buffer, uint32_t offset, uint32_t data);

/**
 * @brief Adds a 16 byte command to the buffer
 * 
 * @param buffer the data to be written to the device
 * @param offset the number of elements already in the buffer
 * @param data the command to be added to the buffer
 * @return the new offset of the buffer
 */
uint32_t scd40_add_command_to_buffer(uint8_t* buffer, uint32_t offset, uint16_t data);

/**
 * @brief Reads the data from the device and removes the checksums 
 * 
 * @param buffer the out parameter where the read data will be stored
 * @param expected_size the expected number of bytes WITHOUT the checksums
 * @param fd the opened file descriptor for the I2C device
 * @return returns an error if one is recieved, NOERR otherwise
 */
int8_t scd40_read_without_crc(uint8_t* buffer, uint16_t expected_size, int* fd);

/**
 * @brief Makes the first two bytes in the buffer into a uint16_t number
 * 
 * @param buffer the data read from the device
 * @return the uint16_t number
 */
uint16_t scd40_read_bytes_as_uint16(uint8_t* buffer);

/**
 * @brief Makes the first two bytes in the buffer into an int16_t number
 * 
 * @param buffer the data read from the device
 * @return the int16_t number
 */
int16_t scd40_read_bytes_as_int16(uint8_t* buffer);

#endif