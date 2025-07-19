#include <MQTTClientPersistence.h>
#include <MQTTReasonCodes.h>
#include <bits/time.h>
#include <bits/types/struct_itimerspec.h>
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
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <regex.h>

#define NUM_THREADS 1
#define CLIENTID "sensor_pub"
#define TOPIC "sensors/data"
#define QOS 1
#define TIMEOUT 10000L
#define FIVE_SECONDS 5000

int pipe_fds[NUM_THREADS][2];
MQTTClient_deliveryToken delivered_token;
volatile sig_atomic_t sigint_recieved = 0;
FILE* log_file;

/**
 * @brief Prints the current timestamp to the log file 
 * 
 */
void print_timestamp(void) {
    time_t raw_time;
    struct tm time_info;
    struct tm* result;

    time(&raw_time);
    result = localtime_r(&raw_time, &time_info);

    if (result == NULL) {
        perror("Failed to get local time");
    } else {
        fprintf(log_file, "Timestamp: %s", asctime(&time_info));
        fflush(log_file);
    }
}

/**
 * @brief Handles when the user interupts the publish cycle
 * 
 * Increments the sigint_recieved flag to tell the main loop to disconnect and
 * clean up
 * 
 * @param signum 
 */
void signal_handler(int signum __attribute__((unused))) {
    sigint_recieved = 1;
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
void delivered(void* context __attribute__((unused)), MQTTClient_deliveryToken token) {
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
int msgarrvd(void* context __attribute__((unused)), char* topic_name, 
            int topic_len __attribute__((unused)), MQTTClient_message* message) {

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
void connlost(void* context __attribute__((unused)), char* cause) {
    print_timestamp();
    fprintf(log_file, "\nConnection Lost!\nCause: %s\n", cause);
    fflush(log_file);
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
        return -1;
    }

    value = regexec(&regex, log_filename, 0, NULL, 0);

    regfree(&regex);

    return value;
}

struct Sensor_Data {
    int error_num;
    int num_data;
    float data[NUM_DATAPOINTS];
};

int create_timer(int* timer_fd, int* epoll_fd, struct epoll_event* event, struct itimerspec* timerspec) {
    if ((*timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
        print_timestamp();
        fprintf(log_file, "Failed to create timerfd, returned with error %d\n", errno);
        fflush(log_file);
        return errno;
    }

    timerspec->it_interval.tv_nsec = 0;
    timerspec->it_interval.tv_sec = 5;
    timerspec->it_value.tv_nsec = 0;
    timerspec->it_value.tv_sec = 5;

    if ((timerfd_settime(*timer_fd, 0, timerspec, NULL)) < 0) {
        print_timestamp();
        fprintf(log_file, "Failed to set timer time, returned with error %d\n", errno);
        fflush(log_file);
        return errno;
    }

    if ((*epoll_fd = epoll_create1(0)) == -1) {
        print_timestamp();
        fprintf(log_file, "failed to create epoll, returned with error %d\n", errno);
        fflush(log_file);
        return errno;
    }

    event->events = EPOLLIN;
    event->data.fd = *timer_fd;
    if ((epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *timer_fd, event)) == -1) {
        print_timestamp();
        fprintf(log_file, "Failed epoll_ctl, returned with error %d\n", errno);
        fflush(log_file);
        return errno;
    }

    return 0;
}

void* sensor_worker(void* arg) {
    int device_status, timer_fd, epoll_fd;
    bool is_ready = false;
    float buffer[NUM_DATAPOINTS];
    int id;
    uint64_t result;
    struct Sensor_Data data;
    struct epoll_event event;
    struct itimerspec timerspec;

    id = *(int *)arg;
    free(arg);

    if ((device_status = create_timer(&timer_fd, &epoll_fd, &event, &timerspec)) != 0) {
        goto exit;
    }

    if ((device_status = device_init(1)) != NOERR) {
        print_timestamp();
        fprintf(log_file, "Unable to initialize device, returned with error %d\n", device_status);
        fflush(log_file);
        goto close_descriptors;
    }

    if ((device_status = start_measurement()) != NOERR) {
        print_timestamp();
        fprintf(log_file, "Failed to start measurements, returned with error %d\n", device_status);
        fflush(log_file);
        goto free_device;
    }

    do {
        if ((device_status = read_data_flag(&is_ready)) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to read data-ready flag, returned with error %d\n", device_status);
            fflush(log_file);
            goto stop_measurements;
        }
    } while(!is_ready);

    while (!sigint_recieved) {
        if ((device_status = epoll_wait(epoll_fd, &event, 1, -1)) == -1) {
            if (errno == EINTR) {continue;}
            fprintf(log_file, "Failed the epoll_wait(), returned with error %d\n", errno);
            goto stop_measurements;
        }

        if ((device_status = read_into_buffer(buffer, NUM_DATAPOINTS)) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to read data into buffer, returned with error %d\n", device_status);
            fflush(log_file);
            goto stop_measurements;
        }

        data.num_data = NUM_DATAPOINTS;
        memcpy(data.data, buffer, NUM_DATAPOINTS * sizeof(float));
        data.error_num = device_status;

        write(pipe_fds[id][1], &data, sizeof(data));

        device_status = read(timer_fd, &result, sizeof(result));
    }

    stop_measurements:
        if ((device_status = stop_measurement()) != NOERR) {
            print_timestamp();
            fprintf(log_file, "Failed to stop measurements, returned with code %d\n", device_status);
            fflush(log_file);
        }
    free_device:
        device_free();
    close_descriptors:
        close(epoll_fd);
        close(timer_fd);
    exit:
        data.error_num = device_status;
        write(pipe_fds[id][1], &data, sizeof(data));
        pthread_exit(NULL);
}


int main(void) {
    //TODO: Move the MQTT server into its own thread
    const char* const log_filename = "log.txt";
    pthread_t threads[NUM_THREADS];
    int epoll_fd = epoll_create1(0);
    float data[NUM_DATAPOINTS] = {0};

    if ((log_file = fopen(log_filename, "a")) == NULL) {
        printf("Could not open log file!\n");
        exit(EXIT_FAILURE);
    }

    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message message = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int client_status = MQTTCLIENT_SUCCESS;

    if ((client_status = MQTTClient_create(&client, ADDRESS, CLIENTID, 
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(log_file, "Failed to create client, returned with code %d\n", client_status);
            fflush(log_file);
            client_status = EXIT_FAILURE;
            goto exit;
    }

    if ((client_status = MQTTClient_setCallbacks(client, NULL, 
        connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(log_file, "Failed to set callbacks, returned with code %d\n", client_status);
            fflush(log_file);
            client_status = EXIT_FAILURE;
            goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20; //keeps the connection alive for 20 seconds
    conn_opts.cleansession = 1; //disregards state info after disconects

    if ((client_status = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        print_timestamp();
        fprintf(log_file, "Failed to connect, returned with code %d\n", client_status);
        fflush(log_file);
        client_status = EXIT_FAILURE;
        goto destroy_exit;
    }


    struct sigaction action;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &action, NULL) != 0) {
        fprintf(stderr, "Failed to initialize sigaction, returned with error %d\n", errno);
        exit(errno);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pipe(pipe_fds[i]) == -1) {
            fprintf(log_file, "Failed to create pipe\n");
            fflush(log_file);
            client_status = EXIT_FAILURE;
            goto disconnect;
        }

        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.u32 = i;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fds[i][0], &event);
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, sensor_worker, id);
    }

    struct epoll_event events[NUM_THREADS];

    while (!sigint_recieved) {
        char* payload = NULL;
        cJSON* root = cJSON_CreateObject();

        int num_ready = epoll_wait(epoll_fd, events, NUM_THREADS, FIVE_SECONDS);

        for (int i = 0; i < num_ready; ++i) {
            int index = events[i].data.u32;
            struct Sensor_Data thread_data;
            read(pipe_fds[index][0], &thread_data, sizeof(thread_data));

            if (thread_data.error_num != 0) {
                goto disconnect;
            }

            switch (index) {
                case 0:
                    memcpy(data, thread_data.data, NUM_DATAPOINTS * sizeof(float));
                    break;
            }
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
                fflush(log_file);
                client_status = EXIT_FAILURE;
                goto disconnect;
        } 

        while(delivered_token != token) {
            usleep(TIMEOUT);
        }

        free(payload);
        payload = NULL;
    }

    //Clean-ups
    disconnect:
        for (int i = 0; i < NUM_THREADS; ++i) {
            pthread_join(threads[i], NULL);
        }

        if (MQTTClient_isConnected(client)) {
            if ((client_status = MQTTClient_disconnect(client, TIMEOUT) != MQTTCLIENT_SUCCESS)) {
                print_timestamp();
                fprintf(log_file, "\nFailed to disconnect, exited with code %d\n", client_status);
                fflush(log_file);
                client_status = EXIT_FAILURE;
            }
        }

    destroy_exit:
        MQTTClient_destroy(&client);

    exit:
        fclose(log_file);
        return client_status;
}

