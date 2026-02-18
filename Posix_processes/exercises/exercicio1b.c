#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
	Para essa versão utilizarei pipes para criar comunicação entre os processos e ambos incrementarem o mesmo valor, facil demais

*/

int main() {

	int x = 10;
	int pipe_fp[2];
	if(pipe(pipe_fp) == -1) {
		perror("pipe");
		return 1;
	}

	pid_t pid = fork();

	if(pid == 0) {
		close(pipe_fp[0]); //fecha leitura
		x += 5;
		printf("Valor no processo filho: %d | PID: %d\n", x, getpid());
		write(pipe_fp[1], &x, sizeof(int));
		printf("Mensagem enviada para o pai\n");

		close(pipe_fp[1]);
		exit(0);
	}

	close(pipe_fp[1]); //fecha a escrita
	read(pipe_fp[0], &x, sizeof(int));
	printf("Mensagem recebida: %d\n", x);
	x += 20;
	printf("Valor no processo pai: %d | PID: %d\n", x, getpid());

	close(pipe_fp[0]);
	printf("Encerrando programa...");
	wait(NULL);


	return 0;
}

// agora essa operação vai funcionar bem, vamos testar
