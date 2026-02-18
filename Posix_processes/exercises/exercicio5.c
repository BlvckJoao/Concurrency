#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>


int main() {

	pid_t pid = fork();

	if(pid == 0){
		printf("Filho de PID: %d executando...\n", getpid());
		sleep(1);
		execlp("ls", "ls", "-l", NULL);
	}

	printf("Pai espera filho terminar...\n");
	pid_t pid_filho = wait(NULL);
	printf("Filhos de PID: %d terminou\n", pid_filho);

	return 0;
}
