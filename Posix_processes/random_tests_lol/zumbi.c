#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

	pid_t pid = fork();

	if(pid == 0){
		printf("Processo filho\n");
		exit(0);
	}else{
		printf("Processo pai\n");
		sleep(10);
		printf("Terminando\n");
	}
	return 0;
}
