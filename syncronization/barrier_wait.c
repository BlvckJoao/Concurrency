#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#define NUM_THREADS 5
#define REPETITIONS 10

atomic_int barrier_counter = 0;
atomic_int rep_counter = 0;

void barrier_wait()  {
	int is_last_thread = 0;
	if(atomic_fetch_add(&barrier_counter, 1) == (NUM_THREADS - 1)) {
		is_last_thread = 1;
	}
	while(barrier_counter < NUM_THREADS);
	if (is_last_thread) {
                atomic_fetch_add(&rep_counter, 1);
                atomic_store(&barrier_counter, 0);
        }
}

void job(uint64_t duration) {
	sleep(duration);
}

void* thread_function(void* args){
	uint64_t thread_id = *(uint64_t*)args;
	
	while(rep_counter < REPETITIONS){
		printf("Thread %lu is running\n", thread_id);
		job(thread_id);
		printf("Thread is finished and reached the barrier\n");
		barrier_wait();
		printf("Thread %lu has passed the barrier\n", thread_id);
	}

	return NULL;
}

int main() {

	srand(time(NULL));
	
	pthread_t threads[NUM_THREADS];
	uint64_t ids[NUM_THREADS];

	printf("Creating %d threads", NUM_THREADS);
	for(size_t i = 0; i < NUM_THREADS; i++){
		ids[i] = 1 + (i ^ rand()) % 9;
		pthread_create(&threads[i], NULL, thread_function, &ids[i]);
	}
	for(size_t i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}