#include <MQTTClientPersistence.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "MQTTClient.h"
#include "../include/device_io.h"
#include "../include/functions.h"
#include <cjson/cJSON.h>
#include <signal.h>
#include "../include/address.h"
#include <time.h>
#include <regex.h>

#define CLIENTID "sensor_pub"
#define TOPIC "sensors/data"
#define QOS 1
#define TIMEOUT 10000L

MQTTClient_deliveryToken delivered_token;
FILE* log_file;

/**
 * @brief Prints the current timestamp to the log file 
 * 
 */
void print_timestamp(void) {
    time_t raw_time;
    struct tm* time_info;
    struct tm* result;

    time(&raw_time);
    time_info = localtime_r(&raw_time, result);

    if (time_info == NULL) {
        perror("Failed to get local time\n");
    }

    fprintf(log_file, "Timestamp: %s", asctime(time_info));
}

/**
 * @brief Handles when the user interupts the publish cycle
 * 
 * Stops the measurements, closes the log file, and frees the sensor device
 * 
 * @param signum 
 */
void signal_handler(int signum) {
    int error;

    if ((error = stop_measurement()) != NOERR) {
        print_timestamp();
        fprintf(log_file, "\nFailed to stop measurements," 
            "exited with error %d\n", error);
        goto safe_exit;
    }

    //printf("\nStopped Measurements\n");

    safe_exit:
        fclose(log_file);
        device_free();
        exit(error);
}

/**
 * @brief Constructs a JSON object with all of the sensor information
 * 
 * Returns the JSON as a string in the inputted json char**
 * 
 * @param root 
 * @param json 
 * @param m_c_1 
 * @param m_c_2_5 
 * @param m_c_4 
 * @param m_c_10 
 * @param humidity 
 * @param temp 
 * @param VOC 
 * @param NOx 
 */
void make_json(cJSON* root, char** json, float m_c_1, float m_c_2_5, float m_c_4, 
    float m_c_10, float humidity, float temp, float VOC, float NOx) {
        cJSON_AddNumberToObject(root, "Mass Concentration PM1.0", m_c_1);
        cJSON_AddNumberToObject(root, "Mass Concentration PM2.5", m_c_2_5);
        cJSON_AddNumberToObject(root, "Mass Concentration PM4.0", m_c_4);
        cJSON_AddNumberToObject(root, "Mass Concentration PM10", m_c_10);
        cJSON_AddNumberToObject(root, "Ambient Humidity", humidity);
        cJSON_AddNumberToObject(root, "Ambient Temperature", temp);
        cJSON_AddNumberToObject(root, "VOC Index", VOC);
        cJSON_AddNumberToObject(root, "NOx Index", NOx);

        char* json_str = cJSON_Print(root);

        *json = malloc(strlen(json_str) + 1);
        strcpy(*json, json_str);

        free(json_str);
        cJSON_Delete(root);
}

/**
 * @brief The delivered callback which is called whenever a payload is delievered
 *          to the server
 * 
 * @param context 
 * @param token 
 */
void delivered(void* context, MQTTClient_deliveryToken token) {
    delivered_token = token;
}

/**
 * @brief The message arrived callback which is used whenever a message is pulled
 * 
 * This function is only used my subscribers so this is unused
 * 
 * @param context 
 * @param topic_name 
 * @param topic_len 
 * @param message 
 * @return int 
 */
int msgarrvd(void* context, char* topic_name, int topic_len, MQTTClient_message* message) {
    printf("Message Arrived\n");
    printf("topic: %s\n", topic_name);
    printf("message: %.*s\n", message->payloadlen, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic_name);
    return 1;
}

/**
 * @brief The connection lost callback which is used whenever the connection to 
 *          the server is lost
 * 
 * @param context 
 * @param cause 
 */
void connlost(void* context, char* cause) {
    print_timestamp();
    fprintf(log_file, "\nConnection Lost!\nCause: %s\n", cause);
}

/**
 * @brief Validates the user's inputted log file
 * 
 * @param log_filename 
 * @return int 
 */
int validate_log_file(const char* const log_filename) {
    regex_t regex;
    //Excludes '/' or '.' in the file name and requires a .txt file
    const char* regex_string = "^[^\\.\\/]+\\.txt$";
    int value;

    if ((value = regcomp(&regex, regex_string, REG_EXTENDED | REG_NOSUB) != 0)) {
        fprintf(stderr, "Regex failed to compile\n");
        exit(EXIT_FAILURE);
    }

    value = regexec(&regex, log_filename, 0, NULL, 0);

    regfree(&regex);

    return value;
}

int main(int argc, char* argv[]) {
    bool reset_flag = false;
    bool is_ready = false;
    char* log_filename = "log.txt";
    int option = 0;

    while ((option = getopt(argc, argv, "rl:")) != -1) {
        switch(option) {
            case 'r': 
                reset_flag = true; 
                break;
            case 'l': 
                if (validate_log_file(optarg) == 0) {
                    log_filename = optarg;
                }
                break;
            case '?': 
                fprintf(stderr, "Unknown option character %s'.\n", optarg);
                exit(EXIT_FAILURE);
        }
    }
    
    

    if ((log_file = fopen(log_filename, "a")) == NULL) {
        printf("Could not open log file!\n");
        exit(1);
    }

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message message = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int client_status = MQTTCLIENT_SUCCESS;
    int device_status = NOERR;



    if ((client_status = MQTTClient_create(&client, ADDRESS, CLIENTID, 
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(log_file, "Failed to create client, returned with code %d\n", client_status);
            client_status = EXIT_FAILURE;
            goto exit;
    }

    if ((client_status = MQTTClient_setCallbacks(client, NULL, 
        connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(log_file, "Failed to set callbacks, returned with code %d\n", client_status);
            client_status = EXIT_FAILURE;
            goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20; //keeps the connection alive for 20 seconds
    conn_opts.cleansession = 1; //disregards state info after disconects

    if ((client_status = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        print_timestamp();
        fprintf(log_file, "Failed to connect, returned with code %d\n", client_status);
        client_status = EXIT_FAILURE;
        goto destroy_exit;
    }

    if ((device_status = device_init(1)) != NOERR) {
        print_timestamp();
        fprintf(log_file, "Failed to initialize device, returned with code %d\n", device_status);
        client_status = EXIT_FAILURE;
        goto destroy_exit;
    }

    if (reset_flag) {
        if ((device_status = reset() != NOERR)) {
            print_timestamp();
            fprintf(log_file, "Failed to reset device, returned with code %d\n", device_status);
            goto free_device;
        }
    }

    if ((device_status = start_measurement()) != NOERR) {
        print_timestamp();
        fprintf(log_file, "Failed to start measurements, returned with code %d\n", device_status);
        client_status = EXIT_FAILURE;
        goto free_device;
    }

    //Handles when then user pressed CTRL + C
    signal(SIGINT, signal_handler);

    do {
        if ((device_status = read_data_flag(&is_ready)) != NOERR) {
            print_timestamp();
            fprintf(log_file,"Failed to get device data-ready flag, returned with code %d\n", device_status);
            client_status = EXIT_FAILURE;
            goto free_device;
        }
    } while (!is_ready);

    while (1) {
        char* payload = NULL;
        cJSON* root = cJSON_CreateObject();
        float data[NUM_DATAPOINTS];

        if ((device_status = read_into_buffer(data, NUM_DATAPOINTS)) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to read device data, returned with code %d\n", device_status);
            client_status = EXIT_FAILURE;
            goto free_device;
        }

        make_json(root, &payload, data[0], data[1], data[2], 
            data[3], data[4], data[5], data[6], data[7]);

        message.payload = payload;
        message.payloadlen = (int)strlen(payload);
        message.qos = QOS;
        message.retained = 0;
        delivered_token = 0;

        if ((client_status = MQTTClient_publishMessage(client, TOPIC, 
            &message, &token)) != MQTTCLIENT_SUCCESS) {
                print_timestamp();
                fprintf(log_file, "Failed to publish message, returned with code %d\n", client_status);
                client_status = EXIT_FAILURE;
                goto free_device;
        } 

        while(delivered_token != token) {
            usleep(TIMEOUT);
        }

        free(payload);
        payload = NULL;
        sleep(5);
    }   

    if ((client_status = MQTTClient_disconnect(client, TIMEOUT)) != MQTTCLIENT_SUCCESS) {
        print_timestamp();
        fprintf(log_file, "Failed to disconnect, returned with code %d\n", client_status);
        client_status = EXIT_FAILURE;
    }

    //Error clean-ups
    free_device:
        if ((device_status = stop_measurement()) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to stop measurements, returned with code %d\n", device_status);
        }
        device_free();

    destroy_exit:
        MQTTClient_destroy(&client);

    exit:
        fclose(log_file);
        return client_status;
}

