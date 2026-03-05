#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/wait.h>

#define NUM_THREADS 3
#define NUM_PROCESS 2

uint64_t counter = 0;
pthread_mutex_t lock;

void* trabalho_thread(void* arg) {
	int id = *(int*)arg;

	for(size_t i = 0; i < 5; i++){
		pthread_mutex_lock(&lock);
		counter++;
		printf("Processo %d | Thread %d | Contador %llu\n", (int)getpid(), id, (unsigned long long)counter);
		pthread_mutex_unlock(&lock);
		usleep(100000);
	}
	return NULL;
}

void executar_processo() {
	pthread_t threads[NUM_THREADS];
	int ids[NUM_THREADS];

	pthread_mutex_init(&lock, NULL);

	for(size_t i = 0; i < NUM_THREADS; i++) {
		ids[i] = i;
		pthread_create(&threads[i], NULL, trabalho_thread, &ids[i]);
	}
	for(size_t i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}
	pthread_mutex_destroy(&lock);
}

int main() {

	for (int i = 0; i < NUM_PROCESS; i++) {
		pid_t pid = fork();

		if (pid == 0) {
			printf("=== Processo filho %d iniciado ===\n", (int)getpid());
			executar_processo();
			exit(0);
		}
	}

	for (int i = 0; i < NUM_PROCESS; i++) {
		wait(NULL);
	}

	printf("Processo pai %d finalizado\n", (int)getpid());
	return 0;
}
