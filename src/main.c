#include "../include/device_io.h"
#include "../include/functions.h"


int main(int argc, char* argv[]) {
    int error;

    error = device_init(1);
    if (error == INIT_FAILED) {
        printf("Device Failed to Initialize\n");
        return error;
    }

    printf("Device initialized successfully!\n");

    error = reset();
    if (error != NOERROR) {
        printf("Device failed to reset");
        device_free();
        return error;
    }

    char name[32];
    error = read_product_name(name);
    if (error != NOERROR) {
        printf("Failed to read product name!\n");
        device_free();
        return error;
    }

    printf("The Product Name is: %s\n", name);

    error = start_measurement();
    if (error != NOERROR) {
        printf("Failed to start measurements");
        device_free();
        return error;
    }

    //usleep(1e+7);

    bool is_ready;
    do {
        error = read_data_flag(&is_ready);
        if (error != NOERROR) {
            printf("Failed to get data!\n");
            device_free();
            return error;
        }
    } while (!is_ready);

    printf("Device Has Data: %d\n", is_ready);

    float m_c_1;
    float m_c_2_5;
    float m_c_4;
    float m_c_10;
    float humidity;
    float temp;
    float VOC;
    float NOx;

    for (int i = 0; i < 10; ++i) {
        error = read_measured_values(&m_c_1, 
            &m_c_2_5, &m_c_4, 
            &m_c_10, &humidity, &temp, 
            &VOC, &NOx);

        if (error != NOERROR) {
            printf("Failed to read data\n");
            device_free();
            return error;
        }

        printf("Temperature in Fahrenheit is: %f\n", temp);
        usleep(2e+6);
    }

    error = stop_measurement();
    if (error != NOERROR) {
        printf("Failed to stop measurements");
        device_free();
        return error;
    }


    device_free();
}