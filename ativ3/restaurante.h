#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_COZINHEIROS (6669 % 4) + 2
#define NUM_FOGOES (6669 % 3) + 2
#define NUM_MESAS ((6669 / 10) % 5) + 5
#define NUM_GARCONS ((6669 / 100) % 3) + 2
#define TEMPO_PREPARO = ((6669 / 100) % 3) + 1

typedef struct pedido {
        int id;
        int mesa_id;
        int prato;
} Pedido;

void* thread_garcom(void* arg);
void* thread_cozinheiro(void* arg);
void* thread_cliente(void* arg);