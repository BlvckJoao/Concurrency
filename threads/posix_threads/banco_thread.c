#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define NUM_TRANSFERENCIAS 100
#define VAL_MAX_TRANSFERENCIA 100
#define SALDO_INICIAL 1000
#define NUM_THREADS 10
#define NUM_CONTAS 5

typedef struct {
        int saldo;
        pthread_mutex_t mutex;
} Conta;

Conta contas[NUM_CONTAS];

void conta_init(conta* a) {
        a->saldo = SALDO_INICIAL;
        a->mutex = PTHREAD_MUTEX_INITIALIZER;
        pthread_mutex_init(&(a->mutex));
}

void* thread_work(void* arg){
        int id = *((int*)arg);
        uint64_t seed = time(NULL) ^ id;
        int transferencia;

        for(size_t i = 0; i < NUM_TRANSFERENCIAS; i++){
                while(transferencia > contas[i].saldo){
                        transferencia = (rand_r(seed) ^ i) % VAL_MAX_TRANSFERENCIA;
                        if(transferencia > contas[i].saldo){
                                continue;
                        }
                        conta
                }
        }
}

//unfinished