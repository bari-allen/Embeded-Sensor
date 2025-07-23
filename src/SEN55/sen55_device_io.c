#include "../../include/SEN55/sen55_device_io.h"

/*******************************************************************************
*                          Function Implementations                            *
*******************************************************************************/

int sen55_device_init(uint32_t adapter_num, int* fd) {
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", adapter_num);
    *fd = open(filename, O_RDWR);

    if (*fd < 0) {
        return INIT_ERR;
    }

    if (ioctl(*fd, I2C_SLAVE, SEN55_ADDRESS) < 0) {
        return INIT_ERR;
    }

    return 0;
}

void sen55_device_free(int* fd) {
    if (*fd >= 0) {
        close(*fd);
    }

    *fd = 0;
}

int8_t sen55_device_write(uint8_t* data, uint16_t count, int* fd) {
    if (write(*fd, data, count) != count) {
        return WRITE_ERR;
    }

    return 0;
}

int8_t sen55_device_read(uint8_t* data, uint16_t count, int* fd) {
    if (read(*fd, data, count) != count) {
        return READ_ERR;
    }

    return 0;
}
