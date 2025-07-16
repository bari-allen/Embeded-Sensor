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

#define CLIENTID "sensor_pub"
#define TOPIC "sensors/data"
#define QOS 1
#define TIMEOUT 10000L

MQTTClient_deliveryToken delivered_token;
FILE* log_file;

void print_timestamp(void) {
    time_t raw_time;
    struct tm* time_info;

    time(&raw_time);
    time_info = localtime(&raw_time);
    fprintf(log_file, "Timestamp: %s", asctime(time_info));
}

//Handles when the user presses CTRL + C to stop the program
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

//constructs the JSON string for the given data
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

void delivered(void* context, MQTTClient_deliveryToken token) {
    //printf("Message with token value: %d delivered\n", token);
    delivered_token = token;
}

//Unused since this is the publication side not the subsriber side
int msgarrvd(void* context, char* topic_name, int topic_len, MQTTClient_message* message) {
    printf("Message Arrived\n");
    printf("topic: %s\n", topic_name);
    printf("message: %.*s\n", message->payloadlen, (char*)message->payload);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topic_name);
    return 1;
}

void connlost(void* context, char* cause) {
    print_timestamp();
    fprintf(log_file, "\nConnection Lost!\nCause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    if ((log_file = fopen("log.txt", "a")) == NULL) {
        printf("Could not open log file!\n");
        exit(1);
    }
    //Handles when then user pressed CTRL + C
    signal(SIGINT, signal_handler);

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message message = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;
    int device_error;



    if ((rc = MQTTClient_create(&client, ADDRESS, CLIENTID, 
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(log_file, "Failed to create client, returned with code %d\n", rc);
            rc = EXIT_FAILURE;
            goto exit;
    }

    if ((rc = MQTTClient_setCallbacks(client, NULL, 
        connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(log_file, "Failed to set callbacks, returned with code %d\n", rc);
            rc = EXIT_FAILURE;
            goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20; //keeps the connection alive for 20 seconds
    conn_opts.cleansession = 1; //disregards state info after disconects

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        print_timestamp();
        fprintf(log_file, "Failed to connect, returned with code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    if ((device_error = device_init(1)) == INIT_ERR) {
        print_timestamp();
        fprintf(log_file, "Failed to initialize device, returned with code %d\n", device_error);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    for (int i = 1; i < argc; ++i) {
        if(strcmp(argv[i], "--reset") == 0) {
            device_error = reset();
            if ((device_error = reset()) != NOERR) {
                print_timestamp();
                fprintf(log_file, "Failed to reset device, returned with code %d\n", device_error);
                goto free_device;
            }
        }
    }

    if ((device_error = start_measurement()) != NOERR) {
        print_timestamp();
        fprintf(log_file, "Failed to start measurements, returned with code %d\n", device_error);
        rc = EXIT_FAILURE;
        goto free_device;
    }

    bool is_ready;
    do {
        if ((device_error = read_data_flag(&is_ready)) != NOERR) {
            print_timestamp();
            fprintf(log_file,"Failed to get device data-ready flag, returned with code %d\n", device_error);
            rc = EXIT_FAILURE;
            goto free_device;
        }
    } while (!is_ready);

    while (1) {
        char* payload;
        cJSON* root = cJSON_CreateObject();
        float data[NUM_DATAPOINTS];

        if ((device_error = read_into_buffer(data, NUM_DATAPOINTS)) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to read device data, returned with code %d\n", device_error);
            rc = EXIT_FAILURE;
            goto free_device;
        }

        make_json(root, &payload, data[0], data[1], data[2], 
            data[3], data[4], data[5], data[6], data[7]);

        message.payload = payload;
        message.payloadlen = (int)strlen(payload);
        message.qos = QOS;
        message.retained = 0;
        delivered_token = 0;

        if ((rc = MQTTClient_publishMessage(client, TOPIC, 
            &message, &token)) != MQTTCLIENT_SUCCESS) {
                print_timestamp();
                fprintf(log_file, "Failed to publish message, returned with code %d\n", rc);
                rc = EXIT_FAILURE;
                goto free_device;
        } else {
            //printf("Waiting for publication of %s\n on topic %s " 
                //"for client with cliendID %s\n", payload, TOPIC, CLIENTID);

                while(delivered_token != token) {
                    usleep(TIMEOUT);
                }
        }

        free(payload);
        payload = NULL;
        sleep(5);
    }   

    if ((rc = MQTTClient_disconnect(client, TIMEOUT)) != MQTTCLIENT_SUCCESS) {
        print_timestamp();
        fprintf(log_file, "Failed to disconnect, returned with code %d\n", rc);
        rc = EXIT_FAILURE;
    }

    free_device:
        if ((device_error = stop_measurement()) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to stop measurements, returned with code %d\n", device_error);
        }
        device_free();

    destroy_exit:
        MQTTClient_destroy(&client);

    exit:
        fclose(log_file);
        return rc;
}

