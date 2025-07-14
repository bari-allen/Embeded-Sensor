#include "../include/functions.h"


int8_t start_measurement(void) {
    int8_t error;
    uint8_t buffer[2];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, START_MEASUREMENT);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    usleep(100000);

    return NOERROR;
}

int8_t stop_measurement(void) {
    int8_t error;
    uint8_t buffer[2];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, STOP_MEASUREMENT);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    usleep(1000000);

    return NOERROR;
}

int8_t read_data_flag(bool* is_ready) {
    int error;
    uint8_t buffer[3];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, DATA_READY_FLAG);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    (void)usleep(100000);

    error = read_without_crc(buffer, 2);
    if (error != 0) {
        return error;
    }

    *is_ready = buffer[1];
    return NOERROR;
}

int8_t read_measured_values(float* mass_concentration_1, float* mass_concentration_2_5, 
                            float* mass_concentration_4, float* mass_concentration_10, 
                            float* humidity, float* temperature, float* VOC, float* NOx) {
    const uint16_t INVALID_UINT = 0xFFFF;
    const int16_t INVALID_INT = 0x7FFF;
    uint8_t retries = 0;
    uint8_t buffer[24];
    
    while (retries < MAX_RETRIES) {
        int8_t error;
        int offset = 0;

        offset = add_command_to_buffer(buffer, offset, READ_VALUES);
        error = device_write(buffer, 2);

        if (error != 0) {
            return error;
        }

        (void)usleep(20000);

        error = read_without_crc(buffer, 16);

        if (error == CRCERROR) {
            ++retries;
            continue;
        }

        if (error != 0) {
            return error;
        }

        break;
    }

    uint16_t mass_concentration_1_uint16 = read_bytes_as_uint16(&buffer[0]);
    uint16_t mass_concentration_2_5_uint16 = read_bytes_as_uint16(&buffer[2]);
    uint16_t mass_concentration_4_uint16 = read_bytes_as_uint16(&buffer[4]);
    uint16_t mass_concentration_10_uint16 = read_bytes_as_uint16(&buffer[6]);
    uint16_t humidity_uint16 = read_bytes_as_uint16(&buffer[8]);
    int16_t temperature_int16 = read_bytes_as_int16(&buffer[10]);
    uint16_t VOC_uint16 = read_bytes_as_uint16(&buffer[12]);
    uint16_t NOx_uint16 = read_bytes_as_uint16(&buffer[14]);
    
    *mass_concentration_1 = mass_concentration_1_uint16 == INVALID_UINT 
                                                        ? NAN 
                                                        : mass_concentration_1_uint16 / 10.0F;
    *mass_concentration_2_5 = mass_concentration_2_5_uint16 == INVALID_UINT 
                                                        ? NAN 
                                                        : mass_concentration_2_5_uint16 / 10.0F;
    *mass_concentration_4 = mass_concentration_4_uint16 == INVALID_UINT 
                                                        ? NAN 
                                                        : mass_concentration_4_uint16 / 10.0F;
    *mass_concentration_10 = mass_concentration_10_uint16 == INVALID_UINT 
                                                        ? NAN 
                                                        : mass_concentration_10_uint16 / 10.0F;

    *humidity = humidity_uint16 == INVALID_UINT 
                                ? NAN 
                                : humidity_uint16/ 100.0F;
    *temperature = temperature_int16 == INVALID_INT 
                                ? NAN 
                                : ((temperature_int16 / 200.0F) * 1.8F) + 32;
    *VOC = VOC_uint16 == INVALID_UINT 
                                ? NAN 
                                : VOC_uint16 / 10.0F;
    *NOx = NOx_uint16 == INVALID_UINT 
                                ? NAN 
                                : NOx_uint16 / 10.0F;

    return NOERROR;
}

int8_t read_into_buffer(float* data) {
    const uint16_t INVALID_UINT = 0xFFFF;
    const int16_t INVALID_INT = 0x7FFF;
    uint8_t retries = 0;
    uint8_t buffer[24];
    int8_t error;
    int offset;
    
    while (retries < MAX_RETRIES) {
        offset = 0;

        offset = add_command_to_buffer(buffer, offset, READ_VALUES);
        error = device_write(buffer, 2);

        if (error != 0) {
            return error;
        }

        (void)usleep(20000);

        if ((error = read_without_crc(buffer, 16)) == CRCERROR) {
            ++retries;
            continue;
        }
        
        break;
    }

    if (error != 0) {
        return error;
    }

    uint16_t mass_concentration_1_uint16 = read_bytes_as_uint16(&buffer[0]);
    uint16_t mass_concentration_2_5_uint16 = read_bytes_as_uint16(&buffer[2]);
    uint16_t mass_concentration_4_uint16 = read_bytes_as_uint16(&buffer[4]);
    uint16_t mass_concentration_10_uint16 = read_bytes_as_uint16(&buffer[6]);
    uint16_t humidity_uint16 = read_bytes_as_uint16(&buffer[8]);
    int16_t temperature_int16 = read_bytes_as_int16(&buffer[10]);
    uint16_t VOC_uint16 = read_bytes_as_uint16(&buffer[12]);
    uint16_t NOx_uint16 = read_bytes_as_uint16(&buffer[14]);

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
    data[4] = humidity_uint16 == INVALID_UINT 
                            ? NAN 
                            : humidity_uint16 / 100.0F;
    data[5] = temperature_int16 == INVALID_INT 
                            ? NAN 
                            : ((temperature_int16 / 200.0F) * 1.8F) + 32;
    data[6] = VOC_uint16 == INVALID_UINT 
                        ? NAN 
                        : VOC_uint16 / 10.0F;
    data[7] = NOx_uint16 == INVALID_UINT 
                        ? NAN 
                        : NOx_uint16 / 10.0F;

    return NOERROR;
}

int8_t read_product_name(char* name) {
    int8_t error;
    uint8_t buffer[48];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, READ_NAME);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    (void)usleep(20000);

    error = read_bytes_as_string(buffer, 32, name);
    
    return error != 0 ? error : NOERROR;
}

int8_t read_serial_number(char* serial_number) {
    int8_t error;
    uint8_t buffer[48];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, READ_SERIAL_NUMBER);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    (void)usleep(20000);

    error = read_bytes_as_string(buffer, 32, serial_number);

    return error != 0 ? error : NOERROR;
}

int8_t read_firmware(uint8_t* firmware_version) {
    int8_t error;
    uint8_t buffer[3];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, READ_FIRMWARE);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    (void)usleep(20000);

    error = read_without_crc(buffer, 2);

    if (error != 0) {
        return error;
    }

    *firmware_version = buffer[0];

    return NOERROR;
}

int8_t reset(void) {
    int8_t error;
    uint8_t buffer[2];
    int offset = 0;

    offset = add_command_to_buffer(buffer, offset, RESET);
    error = device_write(buffer, 2);

    if (error != 0) {
        return error;
    }

    (void)usleep(200000);

    return NOERROR;
}