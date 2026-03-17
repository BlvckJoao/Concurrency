#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

#define NUM_CONECTIONS 2
#define NUM_THREADS 5

atomic_int conections_in_use = 0;
pthread_mutex_t lock;

void* client(void* arg) {
	int id = *(int*)arg;

	printf("[Cliente ID: %d] Esperando conexão...\n", id);
	
	while(conections_in_use < NUM_CONECTIONS){

		pthread_mutex_lock(&lock);
		atomic_fetch_add(&conections_in_use, 1);
		printf("[Cliente ID: %d] Usando conexão (%d/%d em uso)\n"
			, id, conections_in_use, NUM_CONECTIONS);
		usleep(100000);
		pthread_mutex_unlock(&lock);

		printf("[Cliente ID: %d] Conexao finalizada\n", id);
		atomic_fetch_sub(&conections_in_use, 1);
		break;
	}

	return NULL;
}

int main() {
	
	pthread_t threads[NUM_THREADS];
	int ids[NUM_THREADS];
	pthread_mutex_init(&lock, NULL);
	srand(time(NULL));

	for(size_t i = 0; i < NUM_THREADS; i++){
		ids[i] = (rand() ^ i) % 100;
		pthread_create(&threads[i], NULL,  client, (void*)&ids[i]);
	}
	for(size_t i = 0; i < NUM_THREADS; i++){
		pthread_join(threads[i], NULL);
	}

	printf("Servidor esperando");
	return 0;
}
