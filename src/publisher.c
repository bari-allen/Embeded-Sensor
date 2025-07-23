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
#include "scd40_device_io.h"
#include "scd40_functions.h"
#include "sen55_functions.h"
#include <time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <pthread.h>
#include <regex.h>

#define NUM_THREADS 2
#define CLIENTID "sensor_pub"
#define TOPIC "sensors/data"
#define QOS 1
#define TIMEOUT 10000L
#define FIVE_SECONDS 5000
#define SEN55_ADDR 0x69U
#define SCD40_ADDR 0x62U


#define MAKE_VOID(x) ((void* )(uintptr_t)x)
#define MAKE_INT(x) ((int)(uintptr_t)x)
#define LOCK_MUTEX(x) (pthread_mutex_lock(&x))
#define UNLOCK_MUTEX(x) (pthread_mutex_unlock(&x))

const uint8_t DEVICE_ADDRS[NUM_THREADS] = {SCD40_ADDR, SEN55_ADDR};
const uint8_t DATAPOINTS[NUM_THREADS] = {SCD40_DATAPOINTS, SEN55_DATAPOINTS};
int pipe_fds[NUM_THREADS][2];
MQTTClient_deliveryToken delivered_token;
volatile sig_atomic_t sigint_recieved = 0;
FILE* LOG_FILE;
pthread_mutex_t lock;

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
        fprintf(LOG_FILE, "Timestamp: %s", asctime(&time_info));
        fflush(LOG_FILE);
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
int make_json(char** json, float m_c_1, float m_c_2_5, float m_c_4, 
    float m_c_10, float humidity, float temp, float VOC, float NOx, float C02) {

        if (json == NULL) {
            return PNTR_ERR;
        }

        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "Mass Concentration PM1.0", m_c_1);
        cJSON_AddNumberToObject(root, "Mass Concentration PM2.5", m_c_2_5);
        cJSON_AddNumberToObject(root, "Mass Concentration PM4.0", m_c_4);
        cJSON_AddNumberToObject(root, "Mass Concentration PM10", m_c_10);
        cJSON_AddNumberToObject(root, "Ambient Humidity", humidity);
        cJSON_AddNumberToObject(root, "Ambient Temperature", temp);
        cJSON_AddNumberToObject(root, "VOC Index", VOC);
        cJSON_AddNumberToObject(root, "NOx Index", NOx);
        cJSON_AddNumberToObject(root, "CO2", C02);

        char* json_str = cJSON_Print(root);

        *json = malloc(strlen(json_str) + 1);
        strcpy(*json, json_str);

        free(json_str);
        cJSON_Delete(root);

        return NOERR;
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
    fprintf(LOG_FILE, "\nConnection Lost!\nCause: %s\n", cause);
    fflush(LOG_FILE);
}

struct Sensor_Data {
    int num_data;
    float data[SEN55_DATAPOINTS];
};

int create_timer(int* timer_fd, int* epoll_fd, struct epoll_event* event, struct itimerspec* timerspec) {
    if ((*timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
        print_timestamp();
        fprintf(LOG_FILE, "Failed to create timerfd, returned with error %d\n", errno);
        fflush(LOG_FILE);
        return errno;
    }

    timerspec->it_interval.tv_nsec = 0;
    timerspec->it_interval.tv_sec = 5;
    timerspec->it_value.tv_nsec = 0;
    timerspec->it_value.tv_sec = 5;

    if ((timerfd_settime(*timer_fd, 0, timerspec, NULL)) < 0) {
        print_timestamp();
        fprintf(LOG_FILE, "Failed to set timer time, returned with error %d\n", errno);
        fflush(LOG_FILE);
        return errno;
    }

    if ((*epoll_fd = epoll_create1(0)) == -1) {
        print_timestamp();
        fprintf(LOG_FILE, "failed to create epoll, returned with error %d\n", errno);
        fflush(LOG_FILE);
        return errno;
    }

    event->events = EPOLLIN;
    event->data.fd = *timer_fd;
    if ((epoll_ctl(*epoll_fd, EPOLL_CTL_ADD, *timer_fd, event)) == -1) {
        print_timestamp();
        fprintf(LOG_FILE, "Failed epoll_ctl, returned with error %d\n", errno);
        fflush(LOG_FILE);
        return errno;
    }

    return 0;
}

void* sensor_worker(void* arg) {
    int id = MAKE_INT(arg);
    uint8_t address = DEVICE_ADDRS[id];
    const int NUM_DATA = DATAPOINTS[id];
    int device_status, timer_fd, epoll_fd;
    bool is_ready = false;
    float buffer[NUM_DATA];
    uint64_t result;
    struct Sensor_Data data = {};
    struct epoll_event event;
    struct itimerspec timerspec;
    int fd = 0;

    if ((device_status = create_timer(&timer_fd, &epoll_fd, &event, &timerspec)) != 0) {
        goto exit;
    }

    if ((device_status = device_init(1, address, &fd)) != NOERR) {
        print_timestamp();
        fprintf(LOG_FILE, "Unable to initialize device, returned with error %d\n", device_status);
        UNLOCK_MUTEX(lock);
        fflush(LOG_FILE);
        goto close_descriptors;
    }

    LOCK_MUTEX(lock);
    if ((device_status = start_measurement(address, &fd)) != NOERR) {
        UNLOCK_MUTEX(lock);
        print_timestamp();
        fprintf(LOG_FILE, "Failed to start measurements for device %d, returned with error %d\n", DEVICE_ADDRS[id], device_status);
        fflush(LOG_FILE);
        goto free_device;
    }
    UNLOCK_MUTEX(lock);

    while (!sigint_recieved) {
        if ((device_status = epoll_wait(epoll_fd, &event, 1, -1)) == -1) {
            fprintf(LOG_FILE, "Failed the epoll_wait(), returned with error %d\n", errno);
            goto stop_measurements;
        }

        LOCK_MUTEX(lock);

        do {
        if ((device_status = read_data_flag(&is_ready, address, &fd)) != NOERR) {
            UNLOCK_MUTEX(lock);
            print_timestamp();
            fprintf(LOG_FILE, "Failed to read data-ready flag, returned with error %d\n", device_status);
            fflush(LOG_FILE);
            goto stop_measurements;
        }
    } while(!is_ready);

        if ((device_status = read_into_buffer(buffer, NUM_DATA, address, &fd)) != NOERR) {
            UNLOCK_MUTEX(lock);
            print_timestamp();
            fprintf(LOG_FILE, "Failed to read data into buffer, returned with error %d\n", device_status);
            fflush(LOG_FILE);
            goto stop_measurements;
        }
        UNLOCK_MUTEX(lock);

        data.num_data = NUM_DATA;
        memcpy(data.data, buffer, NUM_DATA * sizeof(float));

        write(pipe_fds[id][1], &data, sizeof(data));

        device_status = read(timer_fd, &result, sizeof(result));
    }

    stop_measurements:
        LOCK_MUTEX(lock);
        if ((device_status = stop_measurement(address, &fd)) != NOERR) {
            print_timestamp();
            fprintf(LOG_FILE, "Failed to stop measurements, returned with code %d\n", device_status);
            fflush(LOG_FILE);
        }
        UNLOCK_MUTEX(lock);
    free_device:
        device_free(address, &fd);
    close_descriptors:
        close(epoll_fd);
        close(timer_fd);
    exit:
        close(pipe_fds[id][1]);
        pthread_exit(MAKE_VOID(device_status));
}

int disconnect(MQTTClient* client) {
    int client_status = MQTTCLIENT_SUCCESS;

    if (MQTTClient_isConnected(client)) {
        if ((client_status = MQTTClient_disconnect(*client, TIMEOUT)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(LOG_FILE, "\nFailed to disconnect, exited with code %d\n", client_status);
            fflush(LOG_FILE);
        }
    }

    return client_status;
}


int initialize_connection(MQTTClient* client) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int client_status = MQTTCLIENT_SUCCESS;
    if ((client_status = MQTTClient_create(client, ADDRESS, CLIENTID, 
        MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(LOG_FILE, "Failed to create client, returned with code %d\n", client_status);
            fflush(LOG_FILE);
            return client_status;
        }
    
    
    if ((client_status = MQTTClient_setCallbacks(*client, NULL, 
        connlost, msgarrvd, delivered)) != MQTTCLIENT_SUCCESS) {
            print_timestamp();
            fprintf(LOG_FILE, "Failed to set callbacks, returned with code %d\n", client_status);
            fflush(LOG_FILE);
            return client_status;
        }
    
    conn_opts.keepAliveInterval = 20; //keeps the connection alive for 20 seconds
    conn_opts.cleansession = 1; //disregards state info after disconects

    if ((client_status = MQTTClient_connect(*client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        print_timestamp();
        fprintf(LOG_FILE, "Failed to connect, returned with code %d\n", client_status);
        fflush(LOG_FILE);
        client_status = disconnect(client);
        return client_status;
    }
    
    return client_status;
}

int initialize_sigaction() {
    struct sigaction action;
    action.sa_handler = signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &action, NULL) != 0) {
        fprintf(stderr, "Failed to initialize sigaction, returned with error %d\n", errno);
        return errno;
    }

    return NOERR;
}

int initialize_threads(pthread_t* threads, const int epoll_fd) {
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pipe(pipe_fds[i]) == -1) {
            print_timestamp();
            fprintf(LOG_FILE, "Failed to create pipe\n");
            fflush(LOG_FILE);
            return -1;
        }
        
        struct epoll_event event;
        event.events = EPOLLIN;
        event.data.u32 = i;

        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fds[i][0], &event);
        void* id = MAKE_VOID(i);
        pthread_create(&threads[i], NULL, sensor_worker, id);
    }

    return NOERR;
}

int main(void) {
    //MQTT variables
    MQTTClient client;
    MQTTClient_message message = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int client_status = MQTTCLIENT_SUCCESS;

    //Epoll variables
    int epoll_fd = epoll_create1(0);
    struct epoll_event events[NUM_THREADS];

    //Thread variables
    int active_threads = NUM_THREADS;
    pthread_t threads[NUM_THREADS];
    
    float data[SEN55_DATAPOINTS + SCD40_DATAPOINTS] = {0};

    if ((LOG_FILE = fopen("log.txt", "a")) == NULL) {
        printf("Could not open log file!\n");
        exit(EXIT_FAILURE);
    }

    if ((client_status = initialize_connection(&client)) != MQTTCLIENT_SUCCESS) {
        goto destroy_exit;
    }

    if (initialize_sigaction() != NOERR) {
        disconnect(&client);
        goto destroy_exit;
    }

    if (initialize_threads(threads, epoll_fd) != NOERR) {
        disconnect(&client);
        goto destroy_exit;
    }

    while (active_threads != 0) {
        bool read_data = false;
        char* payload = NULL;

        int num_ready = epoll_wait(epoll_fd, events, NUM_THREADS, FIVE_SECONDS);

        for (int i = 0; i < num_ready; ++i) {
            int index = events[i].data.u32;
            uint8_t address = DEVICE_ADDRS[index];

            struct Sensor_Data thread_data;
            ssize_t closed = read(pipe_fds[index][0], &thread_data, sizeof(thread_data));

            if (closed == 0) {

                void* retval = NULL;
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, pipe_fds[index][0], NULL);
                close(pipe_fds[index][0]);
                pthread_join(threads[index], retval);

                if(MAKE_INT(retval) != NOERR) {
                    print_timestamp();
                    fprintf(LOG_FILE, "Worker returned error with code %d\n", MAKE_INT(retval));
                    fflush(LOG_FILE);
                }

                --active_threads;
                continue;
            }

            switch (address) {
                case SEN55_ADDR:
                    memcpy(data, thread_data.data, SEN55_DATAPOINTS * sizeof(float));
                    read_data = true;
                    break;
                case SCD40_ADDR:
                    memcpy(data + SEN55_DATAPOINTS, thread_data.data, SCD40_DATAPOINTS * sizeof(float));
                    read_data = true;
                    break;
            }
        }

        if (!read_data) {
            continue;
        }

        (void)make_json(&payload, data[0], data[1], data[2], 
            data[3], data[4], data[5], data[6], data[7], data[8]);

        message.payload = payload;
        message.payloadlen = (int)strlen(payload);
        message.qos = QOS;
        message.retained = 0;
        delivered_token = 0;

        if ((client_status = MQTTClient_publishMessage(client, TOPIC, 
            &message, &token)) != MQTTCLIENT_SUCCESS) {
                print_timestamp();
                fprintf(LOG_FILE, "Failed to publish message, returned with code %d\n", client_status);
                fflush(LOG_FILE);
                client_status = EXIT_FAILURE;
                disconnect(&client);
                free(payload);
                payload = NULL;
                goto destroy_exit;
        } 

        while(delivered_token != token) {
            usleep(TIMEOUT);
        }

        free(payload);
        payload = NULL;
    }

    destroy_exit:
        MQTTClient_destroy(&client);
        fclose(LOG_FILE);
        pthread_mutex_destroy(&lock);
        return client_status;
}

