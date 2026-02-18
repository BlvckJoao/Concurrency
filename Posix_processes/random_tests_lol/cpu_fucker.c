#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	long long counter;
	int sleep_us = 0;

	if(argc > 1) {
		sleep_us = atoi(argv[1]);
	}

	printf("CPU consumer iniciado (sleep = %d us)\n", sleep_us);
	printf("PID: %d", getpid());

	while(1) {
		counter++;

		if(counter % 1000000 == 0) {
			printf("Contador: %lld milhões\n", counter / 1000000);
		}

		if(sleep_us > 0) {
			usleep(sleep_us);
		}
	}

	return 0;

}
