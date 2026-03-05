#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define VEC_SIZE 1000000
#define NUM_THREADS 8

typedef struct vec_arg{
	int* sum;
	int* part_a;
	int* part_b;
	int start, end;
}Vec_args;

void* vec_sum(void* args){
    Vec_args* vec_args = (Vec_args*)args;
    int size_partition = vec_args->end - vec_args->start;

    for(size_t i = 0; i < size_partition; i++){
        vec_args->sum[vec_args->start + i] = vec_args->part_a[i] + vec_args->part_b[i];
    }

    return NULL;
}


int* vec_partition(int* vec, int start, int end){
	if(end < start) return NULL;

	int size = end - start;
	int* partition = (int*)malloc(size * sizeof(int));

	for(size_t i = 0; i < size; i++){
		partition[i] = vec[start + i];
	}
	return partition;
}

int** vec_partitions(int* vec) {
        int** partitions = (int**)malloc(NUM_THREADS * sizeof(int*));
        if(partitions == NULL) return NULL;

        for(size_t i = 0; i < NUM_THREADS; i++){
                partitions[i] = vec_partition(vec, i * (VEC_SIZE / NUM_THREADS), (i+1) * (VEC_SIZE / NUM_THREADS));
                if(partitions[i] == NULL) return NULL;
        }

        return partitions;
}

int main() {
        srand(time(NULL));

        Vec_args* vec_args = (Vec_args*)malloc(NUM_THREADS * sizeof(Vec_args));
        if(vec_args == NULL) return EXIT_FAILURE;

        pthread_t threads[NUM_THREADS];

        int* vec_a = (int*)malloc(VEC_SIZE * sizeof(int));
        int* vec_b = (int*)malloc(VEC_SIZE * sizeof(int));

        for(size_t i = 0; i < VEC_SIZE; i++){

                unsigned int seed = time(NULL) ^ i;
                vec_a[i] = rand_r(&seed) % 100;
                vec_b[i] = rand_r(&seed) % 100;
        }

        int** part_a = vec_partitions(vec_a);
        int** part_b = vec_partitions(vec_b);
        int* sum = (int*)malloc(VEC_SIZE * sizeof(int));

        for(size_t i = 0; i < NUM_THREADS; i++){
                vec_args[i].part_a = part_a[i];
                vec_args[i].part_b = part_b[i];
                vec_args[i].sum = sum;
                vec_args[i].start = i * (VEC_SIZE / NUM_THREADS);
                vec_args[i].end = (i+1) * (VEC_SIZE / NUM_THREADS);
        }

        for(size_t i = 0; i < NUM_THREADS; i++){
                pthread_create(&threads[i], NULL, vec_sum, &vec_args[i]);
        }
        for(size_t i = 0; i < NUM_THREADS; i++){
                pthread_join(threads[i], NULL);
        }

        printf("Vector A: ");
        for(size_t i = 0; i < VEC_SIZE; i++){
                printf("%d ", vec_a[i]);
        }
        printf("\nVector B: ");
        for(size_t i = 0; i < VEC_SIZE; i++){
                printf("%d ", vec_b[i]);
        }
        
        printf("\nVector C: ");
        for(size_t i = 0; i < VEC_SIZE; i++){
                printf("%d ", sum[i]);
        }
        printf("\n");
        
        free(vec_a);
        free(vec_b);
        
        for(size_t i = 0; i < NUM_THREADS; i++){
                free(part_a[i]);
                free(part_b[i]);
        }
        free(part_a);
        free(part_b);
        free(sum);
        free(vec_args);

        return EXIT_SUCCESS;
}