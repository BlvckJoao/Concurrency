#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define BACKUP_STACK_SIZE 512*1024
#define CLEANUP_STACK_SIZE 64*1024
#define NOT_DETACHED 0
#define DETACHED 1

pthread_attr_t* config_thread(const char* name, size_t stack_size, int detached){
	
	pthread_attr_t *attr = (pthread_attr_t*)malloc(sizeof(pthread_attr_t));
	pthread_attr_init(attr);

	pthread_attr_setstacksize(attr, stack_size);
	pthread_attr_setdetachstate(attr, detached ? PTHREAD_CREATE_DETACHED : PTHREAD_CREATE_JOINABLE);

	printf("Atributos criados (%s): Tamanho da pilha = %lu bytes, Detach state = %s\n", 
			name, stack_size, detached ? "Detached" : "Joinable");
	return attr;
}

size_t show_stack(const char* name){
	pthread_attr_t attr;
	size_t stack_size;

	pthread_attr_init(&attr);
	pthread_attr_getstacksize(&attr, &stack_size);
	printf("%s: Tamanho da pilha = %lu bytes", name, stack_size);
	pthread_attr_destroy(&attr);

	return stack_size;
}

void* backup(void* args){
	show_stack("Thread Backup");
	printf("Thread de backup executando...\n");

	for(long i = 0; i < 99999999; i++);
	printf("...\n");

	int* backup_rst = (int*)malloc(sizeof(int));
	*backup_rst = rand() % 2;
	printf("Thread de Backup finalizada com resultado: %s\n", *backup_rst ? "Sucesso" : "Falha");
	
	pthread_exit(backup_rst);
}

void* cleanup(void *arg){
    	show_stack("Thread Limpeza");
    	printf("Thread de limpeza de cache executando...\n");
    	int limpeza_counter = 0;
    	while (1){
        	usleep(500000);
        	printf("Limpeza de cache: Etapa %d concluída...\n", ++limpeza_counter);
       		pthread_testcancel(); 
    	}
    	printf("Thread de limpeza de cache finalizada.\n");
    	return NULL;
}

int main(void)
{
    	pthread_t t_backup, t_limpeza;

    	srand(time(NULL));

    	pthread_attr_t *attr_backup = config_thread("Thread Backup", BACKUP_STACK_SIZE, NOT_DETACHED);
    	pthread_create(&t_backup, attr_backup, backup, NULL);
    	pthread_attr_destroy(attr_backup);

    	pthread_attr_t *attr_limpeza = config_thread("Thread Limpeza", CLEANUP_STACK_SIZE, DETACHED);
    	pthread_create(&t_limpeza, attr_limpeza, cleanup, NULL);
    	pthread_attr_destroy(attr_limpeza);

    	int *resultado_backup;
    	pthread_join(t_backup, (void **)&resultado_backup);
    	printf("Resultado do backup: %s\n", (*resultado_backup == 0) ? "Falha" : "Sucesso");
    	free(resultado_backup);

    	sleep(1);
    	pthread_cancel(t_limpeza);
    	printf("Programa principal finalizado.\n");

    	return 0;
}
