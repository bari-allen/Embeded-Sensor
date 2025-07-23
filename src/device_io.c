#include "../include/device_io.h"

/*******************************************************************************
*                          Function Implementations                            *
*******************************************************************************/

int device_init(uint32_t adapter_num, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_device_init(adapter_num, fd);
        case SCD40_ADDRESS:
            return scd40_device_init(adapter_num, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t device_free(uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            sen55_device_free(fd);
            break;
        case SCD40_ADDRESS:
            scd40_device_free(fd);
            break;
        default:
            return ADDR_ERR;
    }

    return NOERR;
}

int8_t device_write(uint8_t* data, uint16_t count, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_device_write(data, count, fd);
        case SCD40_ADDRESS:
            return scd40_device_write(data, count, fd);
        default:
            return ADDR_ERR;
    }
}

int8_t device_read(uint8_t* data, uint16_t count, uint8_t device_addr, int* fd) {
    switch (device_addr) {
        case SEN55_ADDRESS:
            return sen55_device_read(data, count, fd);
        case SCD40_ADDRESS:
            return scd40_device_read(data, count, fd);
        default:
            return ADDR_ERR;
    }
}
