#ifndef ORDER_QUEUE_H
#define ORDER_QUEUE_H

#include <stddef.h>

#define QUEUE_SIZE 50

typedef struct order {
    int id;
    int table_id;
    int dish;
    float price;
} Order;

typedef struct queue {
    Order orders[QUEUE_SIZE];
    int start;
    int end;
} OrderQueue;

OrderQueue* create_queue(void);
int queue_is_empty(OrderQueue* q);
int queue_is_full(OrderQueue* q);
int queue_size(OrderQueue* q);
int queue_insert(OrderQueue* q, Order o);
int queue_remove(OrderQueue* q, Order* rm_value);
void queue_free(OrderQueue** pq);
void queue_print(OrderQueue* q);

#endif