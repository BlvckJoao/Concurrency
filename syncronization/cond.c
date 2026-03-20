#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define BUFFER_SIZE 5

int buffer[BUFFER_SIZE];
int count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_empty = PTHREAD_COND_INITIALIZER;


void* producer(void* arg) {

        for(int i = 0; i < 10; i++){
                pthread_mutex_lock(&mutex);
                while(count == BUFFER_SIZE){
                        pthread_cond_wait(&cond_empty, &mutex);
                }
                buffer[count++] = i;
                printf("Produzido: %d\n", i);

                pthread_cond_signal(&cond_full);
                pthread_mutex_unlock(&mutex);
        }
        return NULL;
}

void* consumer(void *arg) {

        for (int i = 0; i < 10; i++) {
                pthread_mutex_lock(&mutex);
                while (count == 0){
                        pthread_cond_wait(&cond_full, &mutex);
                }
                int item = buffer[--count];
                printf("Consumido: %d\n", item);
                pthread_cond_signal(&cond_empty); 
                pthread_mutex_unlock(&mutex);
        }
        return NULL;
}

int main() {

        pthread_t prod, cons;

        pthread_create(&prod, NULL, producer, NULL);
        pthread_create(&cons, NULL, consumer, NULL);

        pthread_join(prod, NULL);
        pthread_join(cons, NULL);

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond_full);
        pthread_cond_destroy(&cond_empty);

        return 0;
}