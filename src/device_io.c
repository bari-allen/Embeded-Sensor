#include "../include/device_io.h"

static int i2c_bus = 0;

/**
 * @brief Initializes the hardware and software components of the given I2C adapter
 * 
 * @param adapter_num the I2C adapter to initialize
 * @return int 0 is successful or INIT_FAILED if unsuccessful
 */
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

/**
 * @brief Closes the I2C adapter
 * 
 */
void device_free(void) {
    if (i2c_bus >= 0) {
        close(i2c_bus);
    }

    i2c_bus = 0;
}

/**
 * @brief Writes the count number of data from data to the I2C device
 * 
 * @param data the data to be written
 * @param count the amount of data to be written
 * @return 0 if successful or WRITE_FAILED if unsuccessful
 */
int8_t device_write(uint8_t* data, uint16_t count) {
    if (write(i2c_bus, data, count) != count) {
        return WRITE_FAILED;
    }

    return 0;
}

/**
 * @brief Reads the count number of data from the I2C device to data
 * 
 * @param data where the data will be written to
 * @param count the amount of data to be read
 * @return 0 if successful or READ_FAILED otherwise
 */
int8_t device_read(uint8_t* data, uint16_t count) {
    if (read(i2c_bus, data, count) != count) {
        return READ_FAILED;
    }

    return 0;
}
