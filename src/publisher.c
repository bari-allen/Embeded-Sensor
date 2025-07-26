#include <bits/time.h>
#include <bits/types/struct_itimerspec.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <regex.h>
#include <cjson/cJSON.h>
#include "MQTTClient.h"
#include "../include/address.h"
#include "../include/device_io.h"
#include "../include/functions.h"

/*******************************************************************************
*                              Defined Constants                               *
*******************************************************************************/

#define DEVICE_COUNT 2
#define CLIENTID "sensor_pub"
#define TOPIC "sensors/data"
#define QOS 1
#define TIMEOUT 10000L
#define WAIT_TIME 5
#define SEN55_ADDR 0x69U
#define SCD40_ADDR 0x62U
#define ADAPTER_NUM 1

static const uint8_t DEVICE_ADDRS[DEVICE_COUNT] = {SCD40_ADDR, SEN55_ADDR};
static const uint8_t DATAPOINTS[DEVICE_COUNT] = {SCD40_DATAPOINTS, SEN55_DATAPOINTS};

/*******************************************************************************
*                                Macro Functions                               *
*******************************************************************************/

#define MAKE_VOID(x) ((void* )(uintptr_t)x)
#define MAKE_INT(x) ((int)(uintptr_t)x)
#define LOCK_MUTEX(x) (pthread_mutex_lock(&x))
#define UNLOCK_MUTEX(x) (pthread_mutex_unlock(&x))

/*******************************************************************************
*                                Global Variables                              *
*******************************************************************************/

//Globals for threading
volatile sig_atomic_t sigint_recieved = 0;
pthread_mutex_t lock;
int pipe_fds[DEVICE_COUNT][2];

//Globals for the MQTT Server and logging
MQTTClient_deliveryToken delivered_token;
FILE* LOG_FILE;

/*******************************************************************************
*                                    Structs                                   *
*******************************************************************************/

struct Sensor_Data {
    int num_data;
    float* data;
};

/*******************************************************************************
*                            Function Implementations                          *
*******************************************************************************/

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
        fprintf(LOG_FILE, "\nTimestamp: %s", asctime(&time_info));
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
 * @brief Turns the inputted data into a JSON string
 * 
 * Currently only indecies 0-8 are being inputted
 * 
 * @param json the output JSON string variable
 * @param data the collected data from the sensors
 * @return PNTR_ERR if json is NULL, NOERR otherwise
 */
int make_json(char** json, float* data) {

        if (json == NULL) {
            return PNTR_ERR;
        }

        cJSON* root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "Mass Concentration PM1.0", data[0]);
        cJSON_AddNumberToObject(root, "Mass Concentration PM2.5", data[1]);
        cJSON_AddNumberToObject(root, "Mass Concentration PM4.0", data[2]);
        cJSON_AddNumberToObject(root, "Mass Concentration PM10", data[3]);
        cJSON_AddNumberToObject(root, "Ambient Humidity", data[4]);
        cJSON_AddNumberToObject(root, "Ambient Temperature", data[5]);
        cJSON_AddNumberToObject(root, "VOC Index", data[6]);
        cJSON_AddNumberToObject(root, "NOx Index", data[7]);
        cJSON_AddNumberToObject(root, "CO2", data[8]);

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
    fprintf(LOG_FILE, "Connection Lost!\nCause: %s\n", cause);
    fflush(LOG_FILE);
}

/**
 * @brief Create a timer object for each sensor
 * 
 * @param timer_fd the file descriptor for the timer
 * @param epoll_fd the file descriptor for the epoll
 * @param event the epoll_event for the timer event
 * @param timerspec the timer object
 * @return errno if the timer epoll couldn't be initialized, NOERR otherwise
 */
int create_timer(int* timer_fd, int* epoll_fd, struct epoll_event* event, struct itimerspec* timerspec) {
    if ((*timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) == -1) {
        print_timestamp();
        fprintf(LOG_FILE, "Failed to create timerfd, returned with error %d\n", errno);
        fflush(LOG_FILE);
        return errno;
    }

    timerspec->it_interval.tv_nsec = 0;
    timerspec->it_interval.tv_sec = WAIT_TIME;
    timerspec->it_value.tv_nsec = 0;
    timerspec->it_value.tv_sec = WAIT_TIME;

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

    return NOERR;
}

/**
 * @brief A thread for each sensor to collect data
 * 
 * Putting each sensor into their own thread allows data to be read concurrently
 * instead of sequentially 
 * 
 * @param arg the id for the thread (its index for the pipe array)
 * @return the device status when the thread exits
 */
void* sensor_worker(void* arg) {
    int id = MAKE_INT(arg);
    int device_status, timer_fd, epoll_fd;

    //Device info
    int device_fd = 0;
    const uint8_t ADDR = DEVICE_ADDRS[id];
    const int NUM_DATA = DATAPOINTS[id];

    //Device Read Data
    float buffer[NUM_DATA];
    struct Sensor_Data data = {};

    //Timer data
    uint64_t result;
    struct epoll_event event;
    struct itimerspec timerspec;

    if ((device_status = create_timer(&timer_fd, &epoll_fd, 
                                &event, &timerspec)) != 0) {
        goto exit;
    }

    if ((device_status = device_init(ADAPTER_NUM, ADDR, &device_fd)) != NOERR) {
        print_timestamp();
        fprintf(LOG_FILE, "Unable to initialize device %d, " 
                "returned with error %d\n", DEVICE_ADDRS[id], device_status);
        UNLOCK_MUTEX(lock);
        fflush(LOG_FILE);
        goto close_descriptors;
    }

    LOCK_MUTEX(lock);
    if ((device_status = start_measurement(ADDR, &device_fd)) != NOERR) {
        UNLOCK_MUTEX(lock);
        print_timestamp();
        fprintf(LOG_FILE, "Failed to start measurements for device %d,"
                " returned with error %d\n", DEVICE_ADDRS[id], device_status);
        fflush(LOG_FILE);
        goto free_device;
    }
    UNLOCK_MUTEX(lock);

    while (!sigint_recieved) {
        bool is_ready = false;

        if ((device_status = epoll_wait(epoll_fd, 
                                &event, 1, -1)) == -1) {
            fprintf(LOG_FILE, "Failed the epoll_wait() for device %d,"
                    " returned with error %s\n", DEVICE_ADDRS[id], strerror(errno));
            goto stop_measurements;
        }

        LOCK_MUTEX(lock);

        do {
            if ((device_status = read_data_flag(&is_ready, 
                                    ADDR, &device_fd)) != NOERR) {
                UNLOCK_MUTEX(lock);
                print_timestamp();
                fprintf(LOG_FILE, "Failed to read data-ready flag for device %d, "
                        "returned with error %d\n", DEVICE_ADDRS[id], device_status);
                fflush(LOG_FILE);
                goto stop_measurements;
            }
        } while(!is_ready);

        if ((device_status = read_into_buffer(buffer, NUM_DATA, 
                                    ADDR, &device_fd)) != NOERR) {
            UNLOCK_MUTEX(lock);
            print_timestamp();
            fprintf(LOG_FILE, "Failed to read data into buffer for device %d, "
                    "returned with error %d\n", DEVICE_ADDRS[id], device_status);
            fflush(LOG_FILE);
            goto stop_measurements;
        }
        UNLOCK_MUTEX(lock);

        data.num_data = NUM_DATA;
        data.data = buffer;

        write(pipe_fds[id][1], &data, sizeof(data));

        device_status = read(timer_fd, &result, sizeof(result));
    }

    stop_measurements:
        LOCK_MUTEX(lock);
        if ((device_status = stop_measurement(ADDR, &device_fd)) != NOERR) {
            print_timestamp();
            fprintf(LOG_FILE, "Failed to stop measurements for device %d, "
                    "returned with code %d\n", DEVICE_ADDRS[id], device_status);
            fflush(LOG_FILE);
        }
        UNLOCK_MUTEX(lock);
    free_device:
        device_free(ADDR, &device_fd);
    close_descriptors:
        close(epoll_fd);
        close(timer_fd);
    exit:
        close(pipe_fds[id][1]);
        pthread_exit(MAKE_VOID(device_status));
}

/**
 * @brief Disconnects the client from the MQTT server
 * 
 * @param client
 * @return whether the client could be disconnected
 */
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

/**
 * @brief Initializes the connection of the client
 * 
 * @param client 
 * @return whether the client could be initialized and connect to the server
 */
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

/**
 * @brief Initializes the sigaction to the signal handler
 * 
 * @return errno if the signal handler couldn't be bound to sigaction, NOERR otherwise
 */
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

/**
 * @brief Initializes the threads with their respective indecies
 * 
 * @param threads the array of sensor threads
 * @param epoll_fd the epoll file descriptor for binding the thread pipes to the epoll
 * @return if the threads could be initialized correctly
 */
int initialize_threads(pthread_t* threads, const int epoll_fd) {
    pthread_mutex_init(&lock, NULL);

    for (int i = 0; i < DEVICE_COUNT; ++i) {
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
    struct epoll_event events[DEVICE_COUNT];

    //Thread variables
    int active_threads = DEVICE_COUNT;
    pthread_t threads[DEVICE_COUNT];
    bool connection_terminated = false;
    
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

        int num_ready = epoll_wait(epoll_fd, events, DEVICE_COUNT, -1);

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
                    memcpy(data + SEN55_DATAPOINTS, thread_data.data, 
                            SCD40_DATAPOINTS * sizeof(float));
                    read_data = true;
                    break;
            }
        }

        if (!read_data || connection_terminated) {
            continue;
        }

        (void)make_json(&payload, data);

        message.payload = payload;
        message.payloadlen = (int)strlen(payload);
        message.qos = QOS;
        message.retained = 0;
        delivered_token = 0;

        //This is technically blocking but since the data comes every 5 seconds
        //it doesn't matter too much
        if ((client_status = MQTTClient_publishMessage(client, TOPIC, 
            &message, &token)) != MQTTCLIENT_SUCCESS) {
                print_timestamp();
                fprintf(LOG_FILE, "Failed to publish message, "
                        "returned with code %d\n", client_status);
                fflush(LOG_FILE);
                disconnect(&client);
                sigint_recieved = 1;
                connection_terminated = true;
                goto free_payload;
        } 

        while(delivered_token != token) {
            usleep(TIMEOUT);
        }

        free_payload:
            free(payload);
            payload = NULL;
    }

    destroy_exit:
        MQTTClient_destroy(&client);
        fclose(LOG_FILE);
        pthread_mutex_destroy(&lock);
        return client_status;
}