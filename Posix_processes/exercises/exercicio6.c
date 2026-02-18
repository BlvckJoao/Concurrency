#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX 100

int main(){

    char msg[MAX];

    printf("Digite o comando desejado: ");
    fgets(msg, MAX, stdin);

    msg[strcspn(msg, "\n")] = 0;

    char* token[MAX];
    const char* delimiter = " ";
    size_t counter = 0;

    token[counter] = strtok(msg, delimiter);

    while(token[counter] != NULL) {
        counter++;
        token[counter] = strtok(NULL, delimiter);
    }

    pid_t pid = fork();

    if(pid == 0){
        printf("Filho executando..\n");
        sleep(1);
        execvp(token[0], token);
        perror("Erro no execvp");
        exit(1);
    }

    printf("Pai espera fim do filho...\n");
    wait(NULL);

    return 0;
}
