#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void work(void) {
    printf("Filho PID = %d, Pai PID = %d\n", getpid(), getppid());
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <n>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "n deve ser > 0\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < n; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            work();
            _exit(EXIT_SUCCESS);
        } 
        else if (pid < 0) {
            perror("fork");
            _exit(EXIT_FAILURE);
        }
    }
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return 0;
}
