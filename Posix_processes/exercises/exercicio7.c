#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {

	int pipe_pf[2];
	char* msg_recebida;

	if(pipe(pipe_pf) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid = fork();

	if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if(pid == 0) {
		close(pipe_pf[0]);

		char* msg = "Olha a mensagem !!!";
		printf("Processo filho de PID: %d enviando mensagem %s ao pai\n", getpid(), msg);
		write(pipe_pf[1], msg, strlen(msg) + 1);

		close(pipe_pf[1]);
		printf("Mensagem enviada, encerrando processo...\n");
		sleep(5);
		exit(0);
	}

	close(pipe_pf[1]);

	printf("Pai de pid %d espera mensagem do filho\n", getpid());
	read(pipe_pf[0], msg_recebida, sizeof(msg_recebida));
	printf("Pai recebeu a mensagem do filho %s\n", msg_recebida);

	close(pipe_pf[0]);

	printf("Processo pai aguarda o processo filho terminar...\n");
	wait(NULL);
	printf("Processo pai finalizado...\n");

	return 0;
}
