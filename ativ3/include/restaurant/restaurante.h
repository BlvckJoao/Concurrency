#ifndef RESTAURANTE_H
#define RESTAURANTE_H

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#include "queue.h"

#define NUM_CHEFS (6669 % 4) + 2
#define NUM_STOVES (6669 % 3) + 2
#define NUM_TABLES ((6669 / 10) % 5) + 5
#define NUM_WAITERS ((6669 / 100) % 3) + 2
#define PREP_TIME ((6669 / 100) % 3) + 1

typedef struct table {
        int id;
        int occupied;
        int ready;
        
        pthread_mutex_t mutex;
        pthread_cond_t cond;
        pthread_cond_t food_ready;
} Table;

typedef struct monitor {
        Table tables[NUM_TABLES];

        OrderQueue* order_queue;
        sem_t sem_stoves;
        pthread_mutex_t order_mutex;
        pthread_cond_t order_has_item;
        pthread_cond_t order_has_space;

        OrderQueue* client_queue;
        pthread_mutex_t client_mutex;
        pthread_cond_t client_has_item;
        pthread_cond_t client_has_space;

        int restaurant_open;
        float invoicing;
        int total_orders;
        pthread_mutex_t stats_mutex;

        pthread_barrier_t barrier;
} RestaurantMonitor;

void monitor_initialize(RestaurantMonitor* rm);
void monitor_destroy(RestaurantMonitor* rm);

void produce_order(RestaurantMonitor* rm, int producer_id, Order order);
void produce_client(RestaurantMonitor* rm, int client_id, Order order);
int consume_order(RestaurantMonitor* rm, Order* order);

void* thread_client(void* arg);
void* thread_cook(void* arg);
void* thread_waiter(void* arg);

#endif