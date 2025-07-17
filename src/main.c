#include "../include/device_io.h"
#include "../include/functions.h"
#include <signal.h>
#include <strings.h>

void signal_handler(int signum) {
    (void)signum;
    
    int error;

    printf("\nClosing I2C File\n");

    error = stop_measurement();
    if (error != NOERR) {
        printf("Failed to stop measurements");
        device_free();
        exit(error);
    }

    printf("Stopped  Measurements\n");

    device_free();
    exit(0);
}

int main(int argc, char* argv[]) {
    //Handles the CTRL + C signal
    signal(SIGINT, signal_handler);

    int error;

    error = device_init(1);
    if (error == INIT_ERR) {
        printf("Device Failed to Initialize\n");
        return error;
    }

    for (int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--reset") == 0) {
            error = reset();
            if (error != NOERR) {
                printf("Device failed to reset");
                device_free();
                return error;
            }
            printf("Device has been reset!\n");
        }
    }

    char name[32];
    error = read_product_name(name, 32);
    if (error != NOERR) {
        printf("Failed to read product name!\n");
        device_free();
        return error;
    }

    printf("The Product Name is: %s\n", name);

    char serial_number[32];
    error = read_serial_number(serial_number, 32);
    if (error != NOERR) {
        printf("Failed to read serial number!\n");
        device_free();
        return error;
    }

    printf("The Serial Number is: %s\n", serial_number);

    uint8_t firmware_version;
    error = read_firmware(&firmware_version);
    if (error != NOERR) {
        printf("Failed to read firmware version\n");
        device_free();
        return error;
    }

    printf("Firmware version: %d\n", firmware_version);


    error = start_measurement();
    if (error != NOERR) {
        printf("Failed to start measurements");
        device_free();
        return error;
    }

    //usleep(1e+7);

    bool is_ready;
    do {
        error = read_data_flag(&is_ready);
        if (error != NOERR) {
            printf("Failed to get data!\n");
            device_free();
            return error;
        }
    } while (!is_ready);

    float m_c_1;
    float m_c_2_5;
    float m_c_4;
    float m_c_10;
    float humidity;
    float temp;
    float VOC;
    float NOx;

    while(1) {
        error = read_measured_values(&m_c_1, 
            &m_c_2_5, &m_c_4, 
            &m_c_10, &humidity, &temp, 
            &VOC, &NOx);

        if (error != NOERR) {
            printf("Failed to read data\n");
            device_free();
            return error;
        }
        printf("****************************************************************************\n");
        printf("Mass Concentration PM1.0: %f ug/m^3\n", m_c_1);
        printf("Mass Concentration PM2.5: %f ug/m^3\n", m_c_2_5);
        printf("Mass Concentration PM4.0: %f ug/m^3\n", m_c_4);
        printf("Mass Concentration PM10: %f ug/m^3\n", m_c_10);
        printf("Ambient Humidity: %f %%RH\n", humidity);
        printf("Temperature in Fahrenheit is: %f\n", temp);
        printf("VOC Index: %f\n", VOC);
        printf("NOx Index: %f\n", NOx);
        sleep(5);
    }
}