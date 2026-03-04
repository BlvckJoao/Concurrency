#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct args{
	float* a;
	float* b;
	int start;
        int end;
	float* partial;
        int tid;
}Args;

float dot_prod_seq(float* a, float* b, int size){
	float dot_prod = 0;
	for(size_t i = 0; i < size; i++){
		dot_prod += a[i] * b[i];
	}
	return dot_prod;
}

void* dot_prod_thread(void* arg){
        Args* args = (Args*)arg;

        float local = 0.0f;

        for(int i = args->start; i < args->end; i++){
                local += args->a[i] * args->b[i];
        }

        args->partial[args->tid] = local;
        return NULL;
}

int main(int argc, char** argv){
	if(argc != 3) return 1;
	int num_threads = atoi(argv[1]);
	int max_size = atoi(argv[2]);

        while(max_size % num_threads != 0){
                max_size++;
        }

        printf("Tamanho do vetor: %d\n", max_size);
        printf("Número de threads: %d\n", num_threads);

	srand(time(NULL));

        clock_t inicio = clock();
	
	Args* args = (Args*)malloc(num_threads * sizeof(Args));
	if(!args) return 1;
	float* vec_a = (float*)malloc(max_size * sizeof(float));
	float* vec_b = (float*)malloc(max_size * sizeof(float));
	if(!vec_a || !vec_b) return 1;

	pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
        if(!threads) return 1;
	
	for(size_t i = 0; i < max_size; i++){
		unsigned int seed = time(NULL) ^ i;
		vec_a[i] = 10 * ((float)rand_r(&seed) / RAND_MAX);
		vec_b[i] = 10 * ((float)rand_r(&seed) / RAND_MAX);	
	}

	float* partial = (float*)calloc(num_threads, sizeof(float));
        float dot = 0.0f;
	int chunk = max_size / num_threads;

        for(size_t i = 0; i < num_threads; i++){
                args[i].a = vec_a;
                args[i].b = vec_b;
                args[i].start = i * chunk;
                args[i].end = (i + 1) * chunk;
                args[i].partial = partial;
                args[i].tid = i;
                pthread_create(&threads[i], NULL, dot_prod_thread, &args[i]);
        }

	for(size_t i = 0; i < num_threads; i++){
		pthread_join(threads[i], NULL);
	}

        for(size_t i = 0; i < num_threads; i++){
		dot += partial[i];
	}


        clock_t fim = clock();
        float time = ((float)(fim - inicio)) / CLOCKS_PER_SEC;

	printf("Resultado: %.6f\n", dot);
        printf("Tempo de execução: %.6f\n", time);

        free(threads);
        free(partial);
	free(vec_a);
	free(vec_b);
	free(args);

	return 0;
}