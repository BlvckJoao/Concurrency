#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_CHILDREN 3

//Para esse exercicio ppreciso simplesmente gerar N processos filhos, e fazer cada um imprimir
//seu PID e PPID, muito fácil vamo lá

void print_pids() {
	printf("PID: %d | PPID: %d\n", getpid(), getppid());
	sleep(5);
	printf("Encerrando processo %d...\n", getpid());
	return;
}

int main() {

	pid_t pids[NUM_CHILDREN];

	for(size_t i = 0; i < NUM_CHILDREN; i++){
		pids[i] = fork();
		if(pids[i] == 0){
			print_pids();
			exit(0);
		}
	}

	for(size_t i = 0; i < NUM_CHILDREN; i++){
		wait(NULL);
	}
	return 0;
}
