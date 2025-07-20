#ifndef ERRORS_H
#define ERRORS_H

/*No error */
#define NOERR 0
/*Returned when the returned checksum and generated checsum mismatch*/
#define CRC_ERR -1
/*Returned when the inputted exected size is odd*/
#define OFFSET_ERR -2
/*Returned when the I2C device failed to be read*/
#define READ_ERR -3
/*Returned when the I2C device failed to be written*/
#define WRITE_ERR -4
/*Returned when the I2C device failed to be intialized*/
#define INIT_ERR -5
/*Returned when the inputted size mismatches with the expected size*/
#define SIZE_ERR -6
/*Returned when the inputted pointer is invalid*/
#define PNTR_ERR -7

#endif