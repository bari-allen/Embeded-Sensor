#include "../../include/SEN55/sen55_functions.h"

/*******************************************************************************
*                           Function Implementations                           *
*******************************************************************************/

int8_t sen55_start_measurement(int* fd) {
    int8_t error;
    uint8_t buffer[2];
    int offset = 0;

    offset = sen55_add_command_to_buffer(buffer, offset, START_MEASUREMENT);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    usleep(100000);

    return NOERR;
}

int8_t sen55_stop_measurement(int* fd) {
    int8_t error;
    uint8_t buffer[2];
    int offset = 0;

    offset = sen55_add_command_to_buffer(buffer, offset, STOP_MEASUREMENT);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    usleep(1000000);

    return NOERR;
}

int8_t sen55_read_data_flag(bool* is_ready, int* fd) {
    int error;
    uint8_t buffer[3];
    int offset = 0;

    offset = sen55_add_command_to_buffer(buffer, offset, DATA_READY_FLAG);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    (void)usleep(100000);

    error = sen55_read_without_crc(buffer, 2, fd);
    if (error != 0) {
        return error;
    }

    *is_ready = buffer[1];
    return NOERR;
}

int8_t sen55_read_into_buffer(float* data, size_t buffer_size, int* fd) {
    const uint16_t INVALID_UINT = 0xFFFF;
    const int16_t INVALID_INT = 0x7FFF;
    uint8_t retries = 0;
    uint8_t buffer[24];
    int8_t error;
    int offset;
    
    if (buffer_size != SEN55_DATAPOINTS) {
        return SIZE_ERR;
    }

    while (retries < MAX_RETRIES) {
        offset = 0;

        offset = sen55_add_command_to_buffer(buffer, offset, READ_VALUES);
        error = sen55_device_write(buffer, 2, fd);

        if (error != 0) {
            return error;
        }

        (void)usleep(20000);

        if ((error = sen55_read_without_crc(buffer, 16, fd)) == CRC_ERR) {
            ++retries;
            continue;
        }
        
        break;
    }

    if (error != 0) {
        return error;
    }

    uint16_t mass_concentration_1_uint16 = sen55_read_bytes_as_uint16(&buffer[0]);
    uint16_t mass_concentration_2_5_uint16 = sen55_read_bytes_as_uint16(&buffer[2]);
    uint16_t mass_concentration_4_uint16 = sen55_read_bytes_as_uint16(&buffer[4]);
    uint16_t mass_concentration_10_uint16 = sen55_read_bytes_as_uint16(&buffer[6]);
    int16_t humidity_uint16 = sen55_read_bytes_as_int16(&buffer[8]);
    int16_t temperature_int16 = sen55_read_bytes_as_int16(&buffer[10]);
    int16_t VOC_uint16 = sen55_read_bytes_as_int16(&buffer[12]);
    int16_t NOx_uint16 = sen55_read_bytes_as_int16(&buffer[14]);

    data[0] = mass_concentration_1_uint16 == INVALID_UINT 
                                        ? NAN 
                                        : mass_concentration_1_uint16 / 10.0F;
    data[1] = mass_concentration_2_5_uint16 == INVALID_UINT 
                                        ? NAN 
                                        : mass_concentration_2_5_uint16 / 10.0F;
    data[2] = mass_concentration_4_uint16 == INVALID_UINT 
                                        ? NAN 
                                        : mass_concentration_4_uint16 / 10.0F;
    data[3] = mass_concentration_10_uint16 == INVALID_UINT  
                                        ? NAN 
                                        : mass_concentration_10_uint16 / 10.0F;
    data[4] = humidity_uint16 == INVALID_INT 
                            ? NAN 
                            : humidity_uint16 / 100.0F;
    data[5] = temperature_int16 == INVALID_INT 
                            ? NAN 
                            : ((temperature_int16 / 200.0F) * 1.8F) + 32;
    data[6] = VOC_uint16 == INVALID_INT 
                        ? NAN 
                        : VOC_uint16 / 10.0F;
    data[7] = NOx_uint16 == INVALID_INT 
                        ? NAN 
                        : NOx_uint16 / 10.0F;

    return NOERR;
}

int8_t sen55_read_product_name(char* name, size_t name_length, int* fd) {
    int8_t error;
    uint8_t buffer[48];
    int offset = 0;

    if (name_length != MAX_NAME_CHARS) {
        return SIZE_ERR;
    }

    offset = sen55_add_command_to_buffer(buffer, offset, READ_NAME);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    (void)usleep(20000);

    error = sen55_read_bytes_as_string(buffer, 32, name, fd);
    
    return error != 0 ? error : NOERR;
}

int8_t sen55_read_serial_number(char* serial_number, size_t number_length, int* fd) {
    int8_t error;
    uint8_t buffer[48];
    int offset = 0;

    if (number_length != MAX_NAME_CHARS) {
        return SIZE_ERR;
    }

    offset = sen55_add_command_to_buffer(buffer, offset, READ_SERIAL_NUMBER);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    (void)usleep(20000);

    error = sen55_read_bytes_as_string(buffer, 32, serial_number, fd);

    return error != 0 ? error : NOERR;
}

int8_t sen55_read_firmware(uint8_t* firmware_version, int* fd) {
    int8_t error;
    uint8_t buffer[3];
    int offset = 0;

    offset = sen55_add_command_to_buffer(buffer, offset, READ_FIRMWARE);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    (void)usleep(20000);

    error = sen55_read_without_crc(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    *firmware_version = buffer[0];

    return NOERR;
}

int8_t sen55_reset(int* fd) {
    int8_t error;
    uint8_t buffer[2];
    int offset = 0;

    offset = sen55_add_command_to_buffer(buffer, offset, RESET);
    error = sen55_device_write(buffer, 2, fd);

    if (error != 0) {
        return error;
    }

    (void)usleep(200000);

    return NOERR;
}