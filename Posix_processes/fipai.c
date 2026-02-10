#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define NUM_FILHOS 5

void work(){
	printf("Processo filho de PID: %d\n", getpid());
	printf("Processo pai de PID: %d\n\n", getppid());
	while(1);
	return;
}

int main(){
	
	pid_t pids[NUM_FILHOS];

	for(size_t i = 0; i < NUM_FILHOS; i++) {
		pids[i] = fork();

		if(pids[i] == 0){
			work();
			exit(i);
		}
	}

	for(size_t i = 0; i < NUM_FILHOS; i++) {
		waitpid(pids[i], NULL, 0);
	}
}
