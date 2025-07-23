#ifndef SEN55_FUNCTIONS_H
#define SEN55_FUNCTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "sen55_buffer_manip.h"
#include "errors.h"
#include "sen55_device_io.h"
#include <unistd.h>
#include <math.h>

/*******************************************************************************
*                              Defined Constants                               *
*******************************************************************************/

#define MAX_RETRIES 4
#define MAX_NAME_CHARS 32
#define SEN55_DATAPOINTS 8

//Device Address Pointers
#define DATA_READY_FLAG 0x202
#define START_MEASUREMENT 0x21
#define STOP_MEASUREMENT 0x104
#define READ_VALUES 0x3C4
#define READ_NAME 0xD014
#define READ_SERIAL_NUMBER 0xD033
#define READ_FIRMWARE 0xD100
#define RESET 0xD304

/*******************************************************************************
*                           Function Definitions                               *
*******************************************************************************/

/**
 * @brief Transitions the device to Measurement-Mode to allow for the data to be read
 * 
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t sen55_start_measurement(int* fd);

/**
 * @brief Transitions the device to Idle-Mode to stop allowing data to be read
 * 
 * Tells the device to stop allowing data to be read
 * 
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t sen55_stop_measurement(int* fd);

/**
 * @brief Asks the device if it has data ready to be read
 * 
 * @param is_ready the out parameter whether the device has data to be read
 * @return an error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t sen55_read_data_flag(bool* is_ready, int* fd);

/**
 * @brief Reads the data the device has ready into the inputted buffer
 * 
 * Similar to the read_measured_values() function but reads into a buffer instead
 * 
 * @param data a float buffer with size of AT LEAST 8
 * @param buffer_size the size of the data buffer, MUST BE 8 FLOATS
 * @return error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t sen55_read_into_buffer(float* data, size_t buffer_size, int* fd);

/**
 * @brief Reads the name of the device
 * 
 * @param name the out parameter containing the device's name
 * @param name_length the size of the name array, MUST BE 32 CHARACTERS
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t sen55_read_product_name(char* name, size_t name_length, int* fd);

/**
 * @brief Reads the serial number of the I2C device
 * 
 * @param serial_number
 * @param number_length the size of the serial_number array, MUST BE 32 CHARACTERS
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t sen55_read_serial_number(char* serial_number, size_t number_length, int* fd);

/**
 * @brief Reads the firmware version of the I2C device
 * 
 * @param firmware_version 
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t sen55_read_firmware(uint8_t* firmware_versio, int* fd);

/**
 * @brief Software resets the device
 * 
 * Software resets the device which puts it in the same state as after a power reset
 * 
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t sen55_reset(int* fd);

#endif