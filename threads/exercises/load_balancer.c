#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#define NOT_DETACHED 0
#define DETACHED 1
#define BIG_STACK_SIZE 1024*1024
#define SMALL_STACK_SIZE 64*1024

pthread_attr_t create_attributes(const char* thread_name, size_t stack_size, int detached){
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_attr_setstacksize(&attr, stack_size);
	pthread_attr_setdetachstate(&attr, detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);

	printf("Atributos criados %s:\nTamanho da pilha = %lu bytes\nDetach state: %s\n",
		       	thread_name, stack_size, detached ? "Detached" : "Joinable");
	return attr;
}

void print_stack_size(const char* name){
	pthread_attr_t attr;
	size_t stack_size;

	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr, &stack_size);
	printf("%s: Tamanho da pilha: %lu bytes\n", name, stack_size);
	pthread_attr_destroy(&attr);
}

void* thread_processing(void *arg){
	print_stack_size("Thread Processamento");
	printf("Thread Processamento executando...\n");

	for(unsigned long i = 0; i < 99999999999; i++);
	printf("...\n");
	sleep(10);
	printf("Thread finalizada\n");

	return NULL;
}

void* thread_log(void* arg){
	print_stack_size("Thread Log");
	printf("Thread de Log executando...\n");
	int log_counter = 0;

	while(log_counter < 10){
		usleep(300000);
		printf("Log fez seu registro: %d...\n", ++log_counter);
	}
	printf("Thread Log finalizada.\n");
	return NULL;
}

int main(){

	pthread_t proc_thread;
	pthread_t log_thread;

	pthread_attr_t attr_proc = create_attributes("Thread_Processamento", BIG_STACK_SIZE, NOT_DETACHED);
	pthread_create(&proc_thread, &attr_proc, thread_processing, NULL);
	pthread_attr_destroy(&attr_proc);

	pthread_attr_t attr_log = create_attributes("Thread log", SMALL_STACK_SIZE, DETACHED);
	pthread_create(&log_thread, &attr_log, thread_log, NULL);
	pthread_attr_destroy(&attr_log);

	pthread_join(proc_thread, NULL);
	sleep(1);
	printf("Programa principal encerrado...\n");

	return 0;
}
