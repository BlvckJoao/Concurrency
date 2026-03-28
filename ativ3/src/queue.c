#include "../include/restaurant/queue.h"

#include <stdio.h>
#include <stdlib.h>

OrderQueue* create_queue(void) {
    OrderQueue* q = (OrderQueue*)malloc(sizeof(OrderQueue));
    if (!q) return NULL;

    q->start = 0;
    q->end = 0;
    return q;
}

int queue_is_empty(OrderQueue* q) {
    if (!q) return -1;
    return q->start == q->end;
}

int queue_is_full(OrderQueue* q) {
    if (!q) return -1;
    return ((q->end + 1) % QUEUE_SIZE) == q->start;
}

int queue_size(OrderQueue* q) {
    if (!q) return -1;

    if (q->end >= q->start)
        return q->end - q->start;

    return QUEUE_SIZE - q->start + q->end;
}

int queue_insert(OrderQueue* q, Order o) {
    if (!q) return -1;
    if (queue_is_full(q)) return 0;

    q->orders[q->end] = o;
    q->end = (q->end + 1) % QUEUE_SIZE;
    return 1;
}

int queue_remove(OrderQueue* q, Order* rm_value) {
    if (!q) return -1;
    if (queue_is_empty(q)) return 0;

    if (rm_value) {
        *rm_value = q->orders[q->start];
    }

    q->start = (q->start + 1) % QUEUE_SIZE;
    return 1;
}

void queue_free(OrderQueue** pq) {
    if (!pq || !*pq) return;

    free(*pq);
    *pq = NULL;
}

void queue_print(OrderQueue* q) {
    if (!q) {
        printf("[ ]\n");
        return;
    }

    printf("[ ");
    for (int i = q->start; i != q->end; i = (i + 1) % QUEUE_SIZE) {
        printf("%d ", q->orders[i].id);
    }
    printf("]\n");
}