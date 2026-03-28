#include "../include/restaurant/restaurante.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

RestaurantMonitor restaurant_monitor;

void monitor_initialize(RestaurantMonitor* rm) {
        rm->order_queue  = create_queue();
        rm->client_queue = create_queue();

        rm->invoicing = 0.0f;
        rm->total_orders = 0;
        rm->restaurant_open = 1;

        sem_init(&rm->sem_stoves, 0, NUM_STOVES);

        pthread_mutex_init(&rm->order_mutex, NULL);
        pthread_cond_init(&rm->order_has_item, NULL);
        pthread_cond_init(&rm->order_has_space, NULL);

        pthread_mutex_init(&rm->client_mutex, NULL);
        pthread_cond_init(&rm->client_has_item, NULL);
        pthread_cond_init(&rm->client_has_space, NULL);

        pthread_mutex_init(&rm->stats_mutex, NULL);

        pthread_barrier_init(&rm->barrier, NULL, NUM_WAITERS + NUM_CHEFS);

        for (int i = 0; i < NUM_TABLES; i++) {
                rm->tables[i].id = i;
                rm->tables[i].occupied = 0;
                rm->tables[i].ready = 0;
                pthread_mutex_init(&rm->tables[i].mutex, NULL);
                pthread_cond_init(&rm->tables[i].cond, NULL);
                pthread_cond_init(&rm->tables[i].food_ready, NULL);
        }
}

void monitor_destroy(RestaurantMonitor* rm) {
        sem_destroy(&rm->sem_stoves);

        pthread_mutex_destroy(&rm->order_mutex);
        pthread_cond_destroy(&rm->order_has_item);
        pthread_cond_destroy(&rm->order_has_space);

        pthread_mutex_destroy(&rm->client_mutex);
        pthread_cond_destroy(&rm->client_has_item);
        pthread_cond_destroy(&rm->client_has_space);

        pthread_mutex_destroy(&rm->stats_mutex);
        pthread_barrier_destroy(&rm->barrier);

        for (int i = 0; i < NUM_TABLES; i++) {
                pthread_mutex_destroy(&rm->tables[i].mutex);
                pthread_cond_destroy(&rm->tables[i].cond);
                pthread_cond_destroy(&rm->tables[i].food_ready);
        }

        queue_free(&rm->order_queue);
        queue_free(&rm->client_queue);
}

void produce_order(RestaurantMonitor* rm, int producer_id, Order order) {
        pthread_mutex_lock(&rm->order_mutex);

        while (queue_is_full(rm->order_queue)) {
                printf("[Garçom %d] fila cheia, aguardando espaço...\n", producer_id);
                pthread_cond_wait(&rm->order_has_space, &rm->order_mutex);
        }

        queue_insert(rm->order_queue, order);
        printf("[Garçom %d] pedido da mesa %d inserido na fila\n", producer_id, order.table_id);

        pthread_cond_signal(&rm->order_has_item);
        pthread_mutex_unlock(&rm->order_mutex);
}

void produce_client(RestaurantMonitor* rm, int client_id, Order order) {
        pthread_mutex_lock(&rm->client_mutex);

        while (queue_is_full(rm->client_queue)) {
                printf("[Cliente %d] fila de clientes cheia, aguardando espaço...\n", client_id);
                pthread_cond_wait(&rm->client_has_space, &rm->client_mutex);
        }

        queue_insert(rm->client_queue, order);
        printf("[Cliente %d] aguardando atendimento na mesa %d\n", client_id, order.table_id);

        pthread_cond_signal(&rm->client_has_item);
        pthread_mutex_unlock(&rm->client_mutex);
}

int consume_order(RestaurantMonitor* rm, Order* order) {
        pthread_mutex_lock(&rm->order_mutex);

        while (queue_is_empty(rm->order_queue) && rm->restaurant_open) {
                pthread_cond_wait(&rm->order_has_item, &rm->order_mutex);
        }

        if (queue_is_empty(rm->order_queue) && !rm->restaurant_open) {
                pthread_mutex_unlock(&rm->order_mutex);
                return 0;
        }

        queue_remove(rm->order_queue, order);
        pthread_cond_signal(&rm->order_has_space);
        pthread_mutex_unlock(&rm->order_mutex);

        return 1;
}

void* thread_client(void* arg) {
        int client_id = *((int*)arg);
        int table_id = client_id % NUM_TABLES;

        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        Table* table = &restaurant_monitor.tables[table_id];


        pthread_mutex_lock(&table->mutex);
        while (table->occupied) {
                pthread_cond_wait(&table->cond, &table->mutex);
        }
        table->occupied = 1;
        table->ready = 0;
        pthread_mutex_unlock(&table->mutex);

        Order order;
        order.id = client_id;
        order.table_id = table_id;
        order.dish = client_id % 5;
        order.price = (client_id % 5) * 10 + 10;

        produce_client(&restaurant_monitor, client_id, order);

        pthread_mutex_lock(&table->mutex);
        while (!table->ready) {
            pthread_cond_wait(&table->food_ready, &table->mutex);
        }

        clock_gettime(CLOCK_MONOTONIC, &t2);
        double wait_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;

        printf("[Cliente %d] recebeu pedido e está comendo (tempo de espera: %.2f segundos)\n", client_id, wait_time);
        sleep(PREP_TIME + 1);
        printf("[Cliente %d] terminou de comer e vai embora\n", client_id);

        table->occupied = 0;
        table->ready = 0;
        pthread_cond_signal(&table->cond);
        pthread_mutex_unlock(&table->mutex);

        return NULL;
}

void* thread_cook(void* arg) {
        int cook_id = *((int*)arg);
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        printf("[Cozinheiro %d] pronto para cozinhar!\n", cook_id);

        while (1) {
                Order order;

                if (!consume_order(&restaurant_monitor, &order)) {
                        break;
                }

                sem_wait(&restaurant_monitor.sem_stoves);

                printf("[Cozinheiro %d] preparando prato da mesa %d\n", cook_id, order.table_id);
                sleep(PREP_TIME);

                sem_post(&restaurant_monitor.sem_stoves);

                printf("[Cozinheiro %d] prato pronto para a mesa %d\n", cook_id, order.table_id);

                Table* table = &restaurant_monitor.tables[order.table_id];
                pthread_mutex_lock(&table->mutex);
                table->ready = 1;
                pthread_cond_signal(&table->food_ready);
                pthread_mutex_unlock(&table->mutex);

                pthread_mutex_lock(&restaurant_monitor.stats_mutex);
                restaurant_monitor.invoicing += order.price;
                restaurant_monitor.total_orders++;
                pthread_mutex_unlock(&restaurant_monitor.stats_mutex);
        }
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double cook_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
        printf("[Cozinheiro %d] terminou de cozinhar e vai embora (tempo de trabalho: %.2f segundos)\n", cook_id, cook_time);

        return NULL;
}

void* thread_waiter(void* arg) {
        int waiter_id = *((int*)arg);
        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        printf("[Garçom %d] pronto para atender!\n", waiter_id);
        while (1) {

                pthread_mutex_lock(&restaurant_monitor.client_mutex);

                while (queue_is_empty(restaurant_monitor.client_queue) &&
                        restaurant_monitor.restaurant_open) {
                        pthread_cond_wait(&restaurant_monitor.client_has_item, &restaurant_monitor.client_mutex);
                }

                if (queue_is_empty(restaurant_monitor.client_queue) &&
                        !restaurant_monitor.restaurant_open) {
                        pthread_mutex_unlock(&restaurant_monitor.client_mutex);
                        break;
                }

                Order order;
                queue_remove(restaurant_monitor.client_queue, &order);

                pthread_cond_signal(&restaurant_monitor.client_has_space);
                pthread_mutex_unlock(&restaurant_monitor.client_mutex);

                printf("[Garçom %d] pegou pedido mesa %d\n", waiter_id, order.table_id);

                produce_order(&restaurant_monitor, waiter_id, order);
        }
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double wait_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;
        printf("[Garçom %d] terminou de atender e vai embora (tempo de trabalho: %.2f segundos)\n", waiter_id, wait_time);
        return NULL;
}

int main(void) {
        pthread_t clients[NUM_TABLES];
        pthread_t chefs[NUM_CHEFS];
        pthread_t waiters[NUM_WAITERS];

        int client_ids[NUM_TABLES];
        int cook_ids[NUM_CHEFS];
        int waiter_ids[NUM_WAITERS];

        monitor_initialize(&restaurant_monitor);

        printf("Restaurante aberto! Temos %d cozinheiros, %d garçons e %d mesas.\n",
            NUM_CHEFS, NUM_WAITERS, NUM_TABLES);

        for (size_t i = 0; i < NUM_TABLES; i++) {
                client_ids[i] = (int)i + 1;
                pthread_create(&clients[i], NULL, thread_client, &client_ids[i]);
        }

        for (size_t i = 0; i < NUM_CHEFS; i++) {
                cook_ids[i] = (int)i + 1;
                pthread_create(&chefs[i], NULL, thread_cook, &cook_ids[i]);
        }

        for (size_t i = 0; i < NUM_WAITERS; i++) {
                waiter_ids[i] = (int)i + 1;
                pthread_create(&waiters[i], NULL, thread_waiter, &waiter_ids[i]);
        }

        for (size_t i = 0; i < NUM_TABLES; i++) {
                pthread_join(clients[i], NULL);
        }

        restaurant_monitor.restaurant_open = 0;
        pthread_cond_broadcast(&restaurant_monitor.client_has_item);
        pthread_cond_broadcast(&restaurant_monitor.order_has_item);

        for (size_t i = 0; i < NUM_CHEFS; i++) {
                pthread_join(chefs[i], NULL);
        }

        for (size_t i = 0; i < NUM_WAITERS; i++) {
                pthread_join(waiters[i], NULL);
        }

        printf("Restaurante fechado! Faturamento total: R$%.2f\n", restaurant_monitor.invoicing);

        monitor_destroy(&restaurant_monitor);
        return 0;
}