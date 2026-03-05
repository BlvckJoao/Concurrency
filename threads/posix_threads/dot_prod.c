#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct args{
	float* part_a;
	float* part_b;
	int size;
	float* dot_prod;
}Args;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

float dot_prod_seq(float* a, float* b, int size){
	float dot_prod = 0;
	for(size_t i = 0; i < size; i++){
		dot_prod += a[i] * b[i];
	}
	return dot_prod;
}

void* dot_prod_thread(void* args){
	Args* a = (Args*)args;
	float local = 0.0f;

	for(size_t i = 0; i < a->size; i++){
		local += a->part_a[i] * a->part_b[i]; 
	}
	pthread_mutex_lock(&mutex);
	*a->dot_prod += local;
	pthread_mutex_unlock(&mutex);

	return NULL;
}

float* partition(float* vec, int start, int end){
	float* part = (float*)malloc((end - start) * sizeof(float));
	if(!part) return NULL;

	for(size_t i = 0; i < end - start; i++){
		part[i] = vec[i + start]; 
	}
	return part;
}

float** partitions(float* vec, int num_parts, int max_size){
	float** parts = (float**)malloc(num_parts * sizeof(float*));
	if(!parts) return NULL;
	
	for(size_t i = 0; i < num_parts; i++){
		parts[i] = partition(vec, i * (max_size/num_parts), (i+1) * (max_size/num_parts));
	}
	return parts;
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
	
	Args* args = (Args*)malloc(num_threads * sizeof(Args));
	if(!args) return 1;
	float* vec_a = (float*)malloc(max_size * sizeof(float));
	float* vec_b = (float*)malloc(max_size * sizeof(float));
	if(!vec_a || !vec_b) return 1;

    clock_t inicio = clock();

	pthread_t threads[num_threads];
	pthread_mutex_init(&mutex, NULL);
	
	for(size_t i = 0; i < max_size; i++){
		unsigned int seed = time(NULL) ^ i;
		vec_a[i] = 10 * ((float)rand_r(&seed) / RAND_MAX);
		vec_b[i] = 10 * ((float)rand_r(&seed) / RAND_MAX);	
	}

	float** parts_a = partitions(vec_a, num_threads, max_size);
	float** parts_b = partitions(vec_b, num_threads, max_size);
	float dot = 0.0f;

	for(size_t i = 0; i < num_threads; i++){
		args[i].part_a = parts_a[i];
		args[i].part_b = parts_b[i];
		args[i].size = max_size/ num_threads;
		args[i].dot_prod = &dot;

		pthread_create(&threads[i], NULL, dot_prod_thread, &args[i]);
	}

	for(size_t i = 0; i < num_threads; i++){
		pthread_join(threads[i], NULL);
	}

    clock_t fim = clock();
    float time = ((float)(fim - inicio)) / CLOCKS_PER_SEC;

	printf("Resultado: %.6f\n", dot);
    printf("Tempo de execução: %.6f\n", time);

	for(size_t i = 0; i < num_threads; i++){
		free(parts_a[i]);
		free(parts_b[i]);
	}
    free(parts_a);
    free(parts_b);
	free(vec_a);
	free(vec_b);
	free(args);

	return 0;
}