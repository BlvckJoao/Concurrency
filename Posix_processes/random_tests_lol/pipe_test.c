#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

int main()
{
    int pipe_fd[2];
    char buffer[100];

    // 1. Criar pipe
    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // 2. Criar processo filho
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
        // 3.1 Processo filho: escreve no pipe
        close(pipe_fd[0]); // Fecha a extremidade de leitura do pipe
        const char *msg = "Mensagem do processo filho via pipe.";
        write(pipe_fd[1], msg, strlen(msg) + 1);
        close(pipe_fd[1]); // Fecha a extremidade de escrita do pipe
        printf("Processo filho escreveu a mensagem no pipe e ira dormir por 10 segundos...\n");
        sleep(10);
        printf("Processo filho finalizado.\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        // 3.2 Processo pai: lê do pipe
        close(pipe_fd[1]); // Fecha a extremidade de escrita do pipe
        read(pipe_fd[0], buffer, sizeof(buffer));
        printf("Processo pai recebeu: %s\n", buffer);
        close(pipe_fd[0]); // Fecha a extremidade de leitura do pipe
        printf("Processo pai aguardando o termino do processo filho...\n");
        wait(NULL); // Espera o processo filho terminar
    }
    printf("Processo pai finalizado.\n");
    return 0;
}
