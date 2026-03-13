#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdatomic.h>

#define NUM_REQUISITIONS 10
#define MAX_SIMULTANEOUS 3

sem_t semaphore;
atomic_int slots = 0;

void* requisition(void* arg){
	
	int id = *(int*)arg;

	printf("[Req %d] Aguardando Slot... (slots ocupados : %d/%d)\n", id, slots, MAX_SIMULTANEOUS);

	sem_wait(&semaphore);
	
	atomic_fetch_add(&slots, 1);
	printf("[Req %d] Processando... (slots ocupados : %d/%d)\n", id, slots, MAX_SIMULTANEOUS);
	sleep(rand() % 3 + 1);
	printf("[Req %d] concluída\n", id);

	sem_post(&semaphore);
	atomic_fetch_sub(&slots, 1);

	printf("[Req %d] terminada, slot liberado (ocupados %d/%d)\n", id, slots, MAX_SIMULTANEOUS);

	return NULL;
}


int main() {
	
	pthread_t threads[NUM_REQUISITIONS];
	int ids[NUM_REQUISITIONS];

	sem_init(&semaphore, 0, MAX_SIMULTANEOUS);

	for(int i = 0; i < NUM_REQUISITIONS; i++){
		ids[i] = i + 1;
		pthread_create(&threads[i], NULL, requisition, (void*)&ids[i]);
	}

	for(int i = 0; i < NUM_REQUISITIONS; i++){
		pthread_join(threads[i], NULL);
	}

	return 0;
}
