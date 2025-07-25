#ifndef SCD40_FUNCTIONS_H
#define SCD40_FUNCTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "scd40_buffer_manip.h"
#include "../errors.h"
#include "scd40_device_io.h"
#include <unistd.h>
#include <math.h>

/*******************************************************************************
*                              Defined Constants                               *
*******************************************************************************/
#define SCD40_MAX_RETRIES 4
#define SCD40_DATAPOINTS 3

#define SCD40_START_MEASUREMENT 0x21B1
#define SCD40_STOP_MEASUREMENT 0x3F86
#define SCD40_READ_DATA_FLAG 0xE4B8
#define SCD40_READ_VALUES 0xEC05

/*******************************************************************************
*                           Function Definitions                               *
*******************************************************************************/

/**
 * @brief Writes the start_measurement command to the I2C device
 * 
 * @param fd the file descriptor of the I2C device
 * @return an error if one is reached, NOERR otherwise
 */
int8_t scd40_start_measurement(int* fd);

/**
 * @brief Writes the stop_measurement command to the I2C device
 * 
 * @param fd the file descriptor of the I2C device
 * @return an error if one is reached, NOERR otherwise
 */
int8_t scd40_stop_measurement(int* fd);

/**
 * @brief Reads the data ready flag from the I2C device
 * 
 * @param is_ready the out parameter where the data ready flag is stored
 * @param fd the opened file descriptor of the I2C device
 * @return an error if one is reached, NOERR otherwise
 */
int8_t scd40_read_data_flag(bool* is_ready, int* fd);

/**
 * @brief Reads the data from the device into the buffer
 * 
 * @param data the buffer where the data will be stored
 * @param buffer_size the number of datapoints read from the device
 * @param fd the opened file descriptor of the I2C device
 * @return an error if one is reached, NOERR otherwise
 */
int8_t scd40_read_into_buffer(float* data, size_t buffer_size, int* fd);

#endif