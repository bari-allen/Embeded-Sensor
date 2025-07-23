#ifndef SCD40_FUNCTIONS_H
#define SCD40_FUNCTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "scd40_buffer_manip.h"
#include "errors.h"
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

int8_t scd40_start_measurement(int* fd);

int8_t scd40_stop_measurement(int* fd);

int8_t scd40_read_data_flag(bool* is_ready, int* fd);

int8_t scd40_read_into_buffer(float* data, size_t buffer_size, int* fd);

#endif