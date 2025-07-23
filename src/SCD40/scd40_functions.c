#include "../../include/SCD40/scd40_functions.h"

/*******************************************************************************
*                           Function Implementations                           *
*******************************************************************************/

int8_t scd40_start_measurement(int* fd) {
    int8_t error;
    uint8_t buffer[2];
    uint32_t offset = 0;

    offset = scd40_add_command_to_buffer(buffer, offset, SCD40_START_MEASUREMENT);
    if ((error = scd40_device_write(buffer, 2, fd)) != NOERR) {
        return error;
    }

    usleep(2);

    return NOERR;
}

int8_t scd40_stop_measurement(int* fd) {
    int8_t error;
    uint8_t buffer[2];
    uint32_t offset = 0;

    offset = scd40_add_command_to_buffer(buffer, offset, SCD40_STOP_MEASUREMENT);
    if ((error = scd40_device_write(buffer, 2, fd)) != NOERR) {
        return error;
    }

    usleep(1000);

    return NOERR;
}

int8_t scd40_read_data_flag(bool* is_ready, int* fd) {
    int8_t error;
    uint8_t buffer[3];
    uint32_t offset = 0;

    offset = scd40_add_command_to_buffer(buffer, offset, SCD40_READ_DATA_FLAG);
    if ((error = scd40_device_write(buffer, 2, fd)) != NOERR) {
        return error;
    }

    usleep(2);

    if ((error = scd40_read_without_crc(buffer, 2, fd)) != NOERR) {
        return error;
    }

    *is_ready = buffer[1];
    return NOERR;
}

int8_t scd40_read_into_buffer(float* data, size_t buffer_size, int* fd) {
    uint8_t retries = 0;
    uint8_t buffer[9];
    int8_t error;
    uint32_t offset;

    if (buffer_size != SCD40_DATAPOINTS) {
        return SIZE_ERR;
    }

    while (retries < SCD40_MAX_RETRIES) {
        offset = 0;

        offset = scd40_add_command_to_buffer(buffer, offset, SCD40_READ_VALUES);
        if ((error = scd40_device_write(buffer, 2, fd)) != NOERR) {
            return error;
        }

        usleep(2);

        if ((error = scd40_read_without_crc(buffer, 6, fd)) == CRC_ERR) {
            ++retries;
            continue;
        }

        break;
    }

    if (error != NOERR) {
        return error;
    }

    uint16_t co2_concentration = scd40_read_bytes_as_uint16(&buffer[0]);
    int16_t temp_C = scd40_read_bytes_as_int16(&buffer[2]);
    uint16_t humidity = scd40_read_bytes_as_int16(&buffer[4]);

    float temp_CF = -14 + (175 * (temp_C) / (pow(2, 16) - 1));
    float temp_FH = (temp_CF * 1.8) + 32;

    float humidity_F = 100 * ((humidity) / pow(2, 16) - 1);

    data[0] = (float)co2_concentration;
    data[1] = temp_FH;
    data[2] = humidity_F;

    return NOERR;
}

