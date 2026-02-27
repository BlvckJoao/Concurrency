#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	uint64_t* number;
	uint64_t* increments;
}inc_args;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* increment(void* args) {
	inc_args* arg = (inc_args*)args;

	uint64_t* n = arg->number;
	uint64_t inc = *(arg->increments);
	uint64_t local = 0;

	for(size_t i = 0; i < inc; i++){
		local++;
	}

	pthread_mutex_lock(&mutex);
	*(n) += local;
	printf("Numero atual %lu\n", *n);
	pthread_mutex_unlock(&mutex);

	return NULL;
}

int main(int argc, char** argv) {

	if(argc != 3){
		printf("Erro\n");
		return 1;
	}
	
	inc_args* args = (inc_args*)malloc(sizeof(inc_args));
	args->number = (uint64_t*)malloc(sizeof(uint64_t));
	args->increments = (uint64_t*)malloc(sizeof(uint64_t));

	int num_threads = strtoull(argv[2], NULL, 10);
	if(strtoull(argv[1], NULL, 10) % num_threads != 0){
		printf("Erro: Numero de incrementos indivisível\n");
		return 0;
	}
	*(args->increments) = strtoull(argv[1], NULL, 10) / num_threads;

	pthread_t thread[num_threads];

	pthread_mutex_init(&mutex, NULL);

	for(size_t i = 0; i < num_threads; i++) {
		pthread_create(&thread[i], NULL, increment, args);
	}

	for(size_t i = 0; i < num_threads; i++){
		pthread_join(thread[i], NULL);
	}

	printf("Resultado final: %lu\n", *(args->number));
	return 0;
}
