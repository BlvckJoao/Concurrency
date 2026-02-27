#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

void* thread_func(void* args) {
	printf("Thread tarbalhando :)\n");
}

int main(){
	
	pthread_t thread_ids[5];

	for(int i = 0; i < 5; i++){
		pthread_create(&thread_ids[i], NULL, thread_func, NULL);
	}
	for(int i = 0; i < 5; i++){
		pthread_join(thread_ids[i], NULL);
	}
	printf("Thread principal encerrando...\n");
	return 1;
}
