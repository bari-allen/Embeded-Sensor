#include "../include/device_io.h"

static int i2c_bus = 0;


int device_init(uint32_t adapter_num) {
    char filename[20];
    snprintf(filename, 19, "/dev/i2c-%d", adapter_num);
    i2c_bus = open(filename, O_RDWR);

    if (i2c_bus < 0) {
        return INIT_FAILED;
    }

    if (ioctl(i2c_bus, I2C_SLAVE, DEVICE_ADDRESS) < 0) {
        return INIT_FAILED;
    }

    return 0;
}

void device_free(void) {
    if (i2c_bus >= 0) {
        close(i2c_bus);
    }

    i2c_bus = 0;
}

int8_t device_write(uint8_t* data, uint16_t count) {
    if (write(i2c_bus, data, count) != count) {
        return WRITE_FAILED;
    }

    return 0;
}

int8_t device_read(uint8_t* data, uint16_t count) {
    if (read(i2c_bus, data, count) != count) {
        return READ_FAILED;
    }

    return 0;
}
