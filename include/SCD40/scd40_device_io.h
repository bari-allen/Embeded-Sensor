#ifndef SCD40_DEVICE_IO_H
#define SCD40_DEVICE_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "../errors.h"
#include <linux/i2c-dev.h>

/*******************************************************************************
*                              Defined Constants                               *
*******************************************************************************/

#define SCD40_ADDRESS 0x62U

/*******************************************************************************
*                            Function Definitions                              *
*******************************************************************************/

/**
 * @brief Initializes the hardware and software components of the given I2C adapter
 * 
 * @param adapter_num the I2C adapter to initialize
 * @return int 0 is successful or INIT_FAILED if unsuccessful
 */
int scd40_device_init(uint32_t adapter_num, int* fd);

/**
 * @brief Closes the I2C adapter
 * 
 */
void scd40_device_free(int* fd);

/**
 * @brief Writes the count number of data from data to the I2C device
 * 
 * @param data the data to be written
 * @param count the amount of data to be written
 * @return 0 if successful or WRITE_FAILED if unsuccessful
 */
int8_t scd40_device_write(uint8_t* data, uint16_t count, int* fd);

/**
 * @brief Reads the count number of data from the I2C device to data
 * 
 * @param data where the data will be written to
 * @param count the amount of data to be read
 * @return 0 if successful or READ_FAILED otherwise
 */
int8_t scd40_device_read(uint8_t* data, uint16_t count, int* fd);

#endif