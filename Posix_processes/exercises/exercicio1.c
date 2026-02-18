#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/*
	Para esse exercicio minha missão é declarar um int x = 10, 
fazer fork, o processo filho soma 5, imprime o valor e o pid passa para o pai
que soma 20, o valor e o pid são imprimidos e então os processos se encerram

	Bem é muito simples kkkkk, vou até usar uma versão com pipe depois

*/

int main() {

	int x = 10;
	pid_t pid = fork();

	if(pid == 0) {
		x += 5;
		printf("Valor no processo filho: %d | PID: %d\n", x, getpid());
		sleep(1);
		exit(0);
	}

	x += 20;
	printf("Valor no processo pai: %d | PID: %d\n", x, getpid());
	sleep(5);
	printf("Encerrando programa...");
	wait(NULL);


	return 0;
}

// os valores nos processos são diferentes pois eles são cópias, portanto
// o incremento de um não afeta o outro, ez
