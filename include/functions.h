#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "sen55_functions.h"
#include "scd40_functions.h"

/*******************************************************************************
*                           Function Definitions                               *
*******************************************************************************/

/**
 * @brief Transitions the device to Measurement-Mode to allow for the data to be read
 * 
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t start_measurement(uint8_t device_addr, int* fd);

/**
 * @brief Transitions the device to Idle-Mode to stop allowing data to be read
 * 
 * Tells the device to stop allowing data to be read
 * 
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t stop_measurement(uint8_t device_addr, int* fd);

/**
 * @brief Asks the device if it has data ready to be read
 * 
 * @param is_ready the out parameter whether the device has data to be read
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t read_data_flag(bool* is_ready, uint8_t device_addr, int* fd);

/**
 * @brief Reads the data the device has ready into the inputted buffer
 * 
 * Similar to the read_measured_values() function but reads into a buffer instead
 * 
 * @param data a float buffer with size of AT LEAST 8
 * @param buffer_size the size of the data buffer, MUST BE 8 FLOATS
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t read_into_buffer(float* data, size_t buffer_size, uint8_t device_addr, int* fd);

/**
 * @brief Reads the name of the device
 * 
 * Only works the for SEN55 device
 * 
 * @param name the out parameter containing the device's name
 * @param name_length the size of the name array, MUST BE 32 CHARACTERS
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t read_product_name(char* name, size_t name_length, uint8_t device_addr, int* fd);

/**
 * @brief Reads the serial number of the I2C device
 * 
 * @param serial_number
 * @param number_length the size of the serial_number array, MUST BE 32 CHARACTERS
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t read_serial_number(char* serial_number, size_t number_length, uint8_t device_addr, int* fd);

/**
 * @brief Reads the firmware version of the I2C device
 * 
 * @param firmware_version 
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t read_firmware(uint8_t* firmware_version, uint8_t device_addr, int* fd);

/**
 * @brief Software resets the device
 * 
 * Software resets the device which puts it in the same state as after a power reset
 * 
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t reset(uint8_t device_addr, int* fd);

#endif