#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

	pid_t pid = fork();

	if(pid == 0) {
		printf("Filho de PID: %d trabalhando... \n", getpid());
		sleep(2);
		printf("Filho terminou (na real fez porra nenhuma)\n");
		exit(42);
	}

	int status;

	printf("Pai espera filho terminar...(na real ele foi comprar cigarro)\n");
	sleep(20);
	printf("Pai voltou. Milagrosamente.\n");
	pid_t pid_filho = wait(&status);
	printf("Filho de PID: %d terminou\n", pid_filho);

	if(WIFEXITED(status)) {
		printf("Código de saída do filho: %d\n", WEXITSTATUS(status));
	}

	return 0;
}
