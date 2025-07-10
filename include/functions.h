#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdbool.h>
#include <stdint.h>
#include "buffer_manip.h"
#include "device_io.h"
#include <unistd.h>
#include <math.h>

#define DATA_READY_FLAG 0x202
#define START_MEASUREMENT 0x21
#define STOP_MEASUREMENT 0x104
#define READ_VALUES 0x3C4
#define READ_NAME 0xD014
#define RESET 0xD304

int8_t start_measurement(void);
int8_t stop_measurement(void);
int8_t read_data_flag(bool* is_ready);
int8_t read_measured_values(float* mass_concentration_1, float* mass_concentration_2_5, 
                            float* mass_concentration_4, float* mass_concentration_10, 
                            float* humidity, float* temperature, float* VOC, float* NOx);
int8_t read_product_name(char* name);
int8_t reset(void);

#endif