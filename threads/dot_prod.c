#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Estrutura para passar dados para cada thread
typedef struct {
    int* vetor_a;      // Primeiro vetor
    int* vetor_b;      // Segundo vetor
    int start;         // Índice inicial
    int end;           // Índice final
    long long resultado_parcial;  // Resultado calculado por esta thread
} ThreadData;

// Função executada por cada thread
void* calcular_produto_parcial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    data->resultado_parcial = 0;
    
    // Calcula o produto interno para a porção atribuída
    for (int i = data->start; i < data->end; i++) {
        data->resultado_parcial += (long long)data->vetor_a[i] * data->vetor_b[i];
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    // Verifica se os argumentos foram fornecidos
    if (argc != 3) {
        printf("Uso: %s <numero_threads> <tamanho_vetor>\n", argv[0]);
        printf("Exemplo: %s 4 10000000\n", argv[0]);
        return 1;
    }
    
    // Lê os argumentos
    int num_threads = atoi(argv[1]);
    int tamanho = atoi(argv[2]);
    
    // Valida os argumentos
    if (num_threads <= 0) {
        printf("Erro: número de threads deve ser maior que 0\n");
        return 1;
    }
    
    if (tamanho <= 0) {
        printf("Erro: tamanho do vetor deve ser maior que 0\n");
        return 1;
    }
    
    if (num_threads > tamanho) {
        printf("Aviso: número de threads (%d) maior que tamanho (%d)\n", 
               num_threads, tamanho);
        printf("Ajustando para %d threads\n", tamanho);
        num_threads = tamanho;
    }
    
    // Aloca memória para os vetores
    int* vetor_a = (int*)malloc(tamanho * sizeof(int));
    int* vetor_b = (int*)malloc(tamanho * sizeof(int));
    
    if (!vetor_a || !vetor_b) {
        printf("Erro ao alocar memória!\n");
        free(vetor_a);
        free(vetor_b);
        return 1;
    }
    
    // Inicializa os vetores com valores aleatórios
    srand(time(NULL));
    for (int i = 0; i < tamanho; i++) {
        vetor_a[i] = rand() % 100;
        vetor_b[i] = rand() % 100;
    }
    
    // Prepara as threads
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadData* thread_data = (ThreadData*)malloc(num_threads * sizeof(ThreadData));
    
    if (!threads || !thread_data) {
        printf("Erro ao alocar memória para threads!\n");
        free(vetor_a);
        free(vetor_b);
        free(threads);
        free(thread_data);
        return 1;
    }
    
    int elementos_por_thread = tamanho / num_threads;
    
    printf("=================================================\n");
    printf("Cálculo de Produto Interno com Threads\n");
    printf("=================================================\n");
    printf("Tamanho dos vetores: %d elementos\n", tamanho);
    printf("Número de threads: %d\n", num_threads);
    printf("Elementos por thread: ~%d\n", elementos_por_thread);
    printf("=================================================\n");
    
    clock_t inicio = clock();
    
    // Cria as threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].vetor_a = vetor_a;
        thread_data[i].vetor_b = vetor_b;
        thread_data[i].start = i * elementos_por_thread;
        
        // Última thread pega os elementos restantes
        if (i == num_threads - 1) {
            thread_data[i].end = tamanho;
        } else {
            thread_data[i].end = (i + 1) * elementos_por_thread;
        }
        
        pthread_create(&threads[i], NULL, calcular_produto_parcial, &thread_data[i]);
    }
    
    // Aguarda todas as threads terminarem e soma os resultados
    long long produto_interno = 0;
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        produto_interno += thread_data[i].resultado_parcial;
        printf("Thread %d processou elementos [%d - %d): %lld\n", 
               i, thread_data[i].start, thread_data[i].end, 
               thread_data[i].resultado_parcial);
    }
    
    clock_t fim = clock();
    double tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
    
    // Exibe resultados
    printf("=================================================\n");
    printf("Resultado do produto interno: %lld\n", produto_interno);
    printf("Tempo de execução: %.6f segundos\n", tempo);
    printf("=================================================\n");
    
    // Libera memória
    free(vetor_a);
    free(vetor_b);
    free(threads);
    free(thread_data);
    
    return 0;
}