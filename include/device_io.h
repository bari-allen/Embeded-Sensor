#ifndef DEVICE_IO_H
#define DEVICE_IO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c-dev.h>

#define READ_FAILED -1
#define WRITE_FAILED -1
#define INIT_FAILED -1

#define DEVICE_ADDRESS 0x69

int device_init(uint32_t adapter_num);
void device_free(void);
int8_t device_write(uint8_t* data, uint16_t count);
int8_t device_read(uint8_t* data, uint16_t count);

#endif