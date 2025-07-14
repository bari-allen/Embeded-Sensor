#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "buffer_manip.h"
#include "device_io.h"
#include <unistd.h>
#include <math.h>

#define MAX_RETRIES 1

#define DATA_READY_FLAG 0x202
#define START_MEASUREMENT 0x21
#define STOP_MEASUREMENT 0x104
#define READ_VALUES 0x3C4
#define READ_NAME 0xD014
#define READ_SERIAL_NUMBER 0xD033
#define READ_FIRMWARE 0xD100
#define RESET 0xD304

/**
 * @brief Transitions the device to Measurement-Mode to allow for the data to be read
 * 
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t start_measurement(void);

/**
 * @brief Transitions the device to Idle-Mode to stop allowing data to be read
 * 
 * Tells the device to stop allowing data to be read
 * 
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t stop_measurement(void);

/**
 * @brief Asks the device if it has data ready to be read
 * 
 * @param is_ready the out parameter whether the device has data to be read
 * @return an error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t read_data_flag(bool* is_ready);

/**
 * @brief Reads the data the device has ready
 * 
 * @param mass_concentration_1 the Mass Concentration PM1.0
 * @param mass_concentration_2_5 the Mass Concentration PM2.5
 * @param mass_concentration_4 the Mass Concentration PM4
 * @param mass_concentration_10 the Mass Concentration PM10
 * @param humidity the compensated ambient humidity
 * @param temperature the compensated ambiant temperature
 * @param VOC the VOC index
 * @param NOx the NOx index
 * @return an error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t read_measured_values(float* mass_concentration_1, float* mass_concentration_2_5, 
                            float* mass_concentration_4, float* mass_concentration_10, 
                            float* humidity, float* temperature, float* VOC, float* NOx);
/**
 * @brief Reads the data the device has ready into the inputted buffer
 * 
 * Similar to the read_measured_values() function but reads into a buffer instead
 * 
 * @param data a float buffer with size of AT LEAST 8
 * @return error if the device couldn't be written or read from, else NOERROR is returned
 */
int8_t read_into_buffer(float* data);

/**
 * @brief Reads the name of the device
 * 
 * @param name the out parameter containing the device's name
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t read_product_name(char* name);

/**
 * @brief Reads the serial number of the I2C device
 * 
 * @param serial_number
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t read_serial_number(char* serial_number);

/**
 * @brief Reads the firmware version of the I2C device
 * 
 * @param firmware_version 
 * @return an error if the device couldn't be written to or read from, else NOERROR is returned
 */
int8_t read_firmware(uint8_t* firmware_version);

/**
 * @brief Software resets the device
 * 
 * Software resets the device which puts it in the same state as after a power reset
 * 
 * @return an error if the device couldn't be written to, else NOERROR is returned
 */
int8_t reset(void);

#endif