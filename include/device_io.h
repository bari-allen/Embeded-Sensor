#ifndef DEVICE_IO_H
#define DEVICE_IO_H

#include "sen55_device_io.h"
#include "scd40_device_io.h"

/*******************************************************************************
*                            Function Definitions                              *
*******************************************************************************/

/**
 * @brief Initializes the hardware and software components of the given I2C adapter
 * 
 * @param adapter_num the I2C adapter to initialize
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return int 0 is successful or INIT_FAILED if unsuccessful
 */
int device_init(uint32_t adapter_num, uint8_t device_addr, int* fd);

/**
 * @brief Closes the I2C adapter
 * 
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 */
int8_t device_free(uint8_t device_addr, int* fd);

/**
 * @brief Writes the count number of data from data to the I2C device
 * 
 * @param data the data to be written
 * @param count the amount of data to be written
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return 0 if successful or WRITE_FAILED if unsuccessful
 */
int8_t device_write(uint8_t* data, uint16_t count, uint8_t device_addr, int* fd);

/**
 * @brief Reads the count number of data from the I2C device to data
 * 
 * @param data where the data will be written to
 * @param count the amount of data to be read
 * @param device_addr the device's hex address on the I2C bus
 * @param fd the opened file descriptor for the I2C device
 * @return 0 if successful or READ_FAILED otherwise
 */
int8_t device_read(uint8_t* data, uint16_t count, uint8_t device_addr, int* fd);

#endif