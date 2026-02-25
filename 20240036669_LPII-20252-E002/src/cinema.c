#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define FILEIRAS 5
#define ASSENTOS_POR_FILEIRA 10
#define NUM_CLIENTES 20

typedef struct {
    int assentos[FILEIRAS][ASSENTOS_POR_FILEIRA];
    int reservas_sucesso;
    int reservas_falha;
} Sala;

Sala sala;
pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;

void inicializar_sala() {
    for(size_t i = 0; i < FILEIRAS; i++) {
        for(size_t j = 0; j < ASSENTOS_POR_FILEIRA; j++){
            sala.assentos[i][j] = 0;
        }
    }
    sala.reservas_sucesso = 0;
    sala.reservas_falha = 0;
}

int tentar_reserva(int cliente_id, int fileira) {
    //SEÇÃO CRÍTICA
    pthread_mutex_lock(&global_mutex);

    for(size_t j = 0; j < ASSENTOS_POR_FILEIRA - 1; j++) {
        if(sala.assentos[fileira][j] == 0 && sala.assentos[fileira][j+1] == 0){
            sala.assentos[fileira][j] = cliente_id;
            sala.assentos[fileira][j+1] = cliente_id;

            pthread_mutex_unlock(&global_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&global_mutex);
    return 0;
}

void* cliente(void* arg) {

    int cliente_id = *((int*)arg);
    free(arg);
    
    unsigned int seed = time(NULL) ^ cliente_id;

    int fileira_inicial = rand_r(&seed) % FILEIRAS;
    int conseguiu = 0;

    for(int i = 0; i < FILEIRAS; i++) {
        int fileira = (fileira_inicial + i) % FILEIRAS;

        if(tentar_reserva(cliente_id, fileira)) {
            conseguiu = 1;
            break;
        }
        usleep(1000);

    }

    //SEÇÃO CRÍTICA: incrementando sala
    pthread_mutex_lock(&global_mutex);
    if(conseguiu)
        sala.reservas_sucesso++;
    else
        sala.reservas_falha++;
    pthread_mutex_unlock(&global_mutex);

    return NULL;
}

void imprimir_sala() {

    int ocupados = 0;

    printf("\nMAPA DA SALA:\n");

    for(int i = 0; i < FILEIRAS; i++) {
        for(int j = 0; j < ASSENTOS_POR_FILEIRA; j++) {

            if(sala.assentos[i][j] == 0)
                printf(". ");
            else {
                printf("%d ", sala.assentos[i][j]);
                ocupados++;
            }
        }
        printf("\n");
    }

    printf("\nTotal de assentos reservados: %d\n", ocupados);
    printf("Clientes atendidos: %d\n", sala.reservas_sucesso);
    printf("Clientes desistiram: %d\n", sala.reservas_falha);

    printf("Verificação: %d == %d\n",
           ocupados,
           2 * sala.reservas_sucesso);
}

int main() {

    pthread_t threads[NUM_CLIENTES];

    srand(time(NULL));

    pthread_mutex_init(&global_mutex, NULL);

    inicializar_sala();

    for(int i = 0; i < NUM_CLIENTES; i++) {
        int* id = malloc(sizeof(int));
        *id = i + 1;
        pthread_create(&threads[i], NULL, cliente, id);
    }

    for(int i = 0; i < NUM_CLIENTES; i++) {
        pthread_join(threads[i], NULL);
    }

    imprimir_sala();

    pthread_mutex_destroy(&global_mutex);

    return 0;
}