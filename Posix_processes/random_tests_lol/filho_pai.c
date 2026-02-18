#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){

	pid_t pid = fork();

	if(pid == 0) {
		printf("Filho escreve\n");
		sleep(1);
		printf("Filho esperou\n");
		while(1);
	}else{
		printf("Pai escreve\n");
		sleep(1);
		printf("Pai esperou\n");
		while(1);
	}
	return 0;
}
