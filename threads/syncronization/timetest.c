#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <stdbool.h>

#define NUM_THREADS 4
#define ITERACOES 100000000

uint64_t saldo = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
volatile bool lock_flag = false;

void spinlock_acquire() {
    while (__sync_lock_test_and_set(&lock_flag, 1));
}

void spinlock_release() {
    __sync_lock_release(&lock_flag);
}

void* atualizar_mutex_curto(void* arg){
        int id = *(int*)arg;
        uint64_t local = 0;
        printf("[Thread ID: %d] executando\n", id);

        for(size_t i = 0; i < ITERACOES; i++){
                local++;                
        }
        pthread_mutex_lock(&mutex);
        saldo += local;
        pthread_mutex_unlock(&mutex);

        return NULL;
}


void* atualizar_mutex_longo(void* arg){
        int id = *(int*)arg;
        uint64_t local = 0;
        printf("[Thread ID: %d] executando\n", id);

        for(size_t i = 0; i < ITERACOES; i++){
                local++;                
        }
        pthread_mutex_lock(&mutex);
        saldo += local;
        usleep(1);
        pthread_mutex_unlock(&mutex);

        return NULL;
}

void* atualizar_spinlock_curto(void* arg){
        int id = *(int*)arg;
        uint64_t local = 0;
        printf("[Thread ID: %d] executando\n", id);

        for(size_t i = 0; i < ITERACOES; i++){
                local++;                
        }
        spinlock_acquire();
        saldo += local;
        spinlock_release();

        return NULL;
}


void* atualizar_spinlock_longo(void* arg){
        int id = *(int*)arg;
        uint64_t local = 0;
        printf("[Thread ID: %d] executando\n", id);

        for(size_t i = 0; i < ITERACOES; i++){
                local++;                
        }

        spinlock_acquire();
        saldo += local;
        usleep(1);
        spinlock_release();

        return NULL;
}

void medir_tempo(void *(*func)(void*), const char* desc){
        struct timespec time_start, time_end;
        struct rusage usage_start, usage_end;
        pthread_t threads[NUM_THREADS];

        saldo = 0;
        lock_flag = false;
        int ids[NUM_THREADS];

        clock_gettime(CLOCK_MONOTONIC, &time_start);
        getrusage(RUSAGE_SELF, &usage_start);

        for(size_t i = 0; i < NUM_THREADS; i++){
                ids[i] = i;
                pthread_create(&threads[i], NULL, func, &ids[i]);
        }
        for(size_t i = 0; i < NUM_THREADS; i++){
                pthread_join(threads[i], NULL);
        }

        clock_gettime(CLOCK_MONOTONIC, &time_end);
        getrusage(RUSAGE_SELF, &usage_end);

        double tempo_real = (time_end.tv_sec - time_start.tv_sec) * 1000.0 +
                        (time_end.tv_nsec - time_start.tv_nsec) / 1000000.0;

        double cpu_user = (usage_end.ru_utime.tv_sec - usage_start.ru_utime.tv_sec) * 1000.0 +
                        (usage_end.ru_utime.tv_usec - usage_start.ru_utime.tv_usec) / 1000.0;

        double cpu_system = (usage_end.ru_stime.tv_sec - usage_start.ru_stime.tv_sec) * 1000.0 +
                                (usage_end.ru_stime.tv_usec - usage_start.ru_stime.tv_usec) / 1000.0;

        double cpu_total = cpu_user + cpu_system;

        printf("\n========== %s ==========\n", desc);
        printf("Tempo real: %.2f ms\n", tempo_real);
        printf("Tempo CPU (user): %.2f ms\n", cpu_user);
        printf("Tempo CPU (system): %.2f ms\n", cpu_system);
        printf("Tempo CPU total: %.2f ms\n", cpu_total);
        printf("Saldo final: %ld\n", saldo);
        printf("===============================\n");
}

int main(void)
{
        printf("Iniciando testes de Mutex e Spinlock...\n");
        printf("====== Seção Crítica Curta ======\n\n");
        medir_tempo(atualizar_mutex_curto, "Mutex - Seção Curta");
        medir_tempo(atualizar_spinlock_curto, "Spinlock - Seção Curta");
        printf("\n====== Seção Crítica Longa ======\n\n");
        medir_tempo(atualizar_mutex_longo, "Mutex - Seção Longa");
        medir_tempo(atualizar_spinlock_longo, "Spinlock - Seção Longa");
        printf("Testes concluídos.\n");

        return 0;
}