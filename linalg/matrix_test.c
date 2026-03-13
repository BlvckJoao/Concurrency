#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define DIMENSION 1000
#define NUM_THREADS 4

typedef struct matrix {
	float** values;
	size_t m;
	size_t n;
}Matrix;

typedef struct matrix_mul_args {
	Matrix* a;
	Matrix* b;
	Matrix* c;
	size_t row_start;
	size_t row_end;
}MatrixMulArgs;

Matrix* create_matrix(size_t m, size_t n) {
	
	Matrix* matrix = (Matrix*)malloc(sizeof(Matrix));

	matrix->values = (float**)malloc(m * sizeof(float));
	if(matrix->values == NULL) return NULL;
	for(size_t i = 0; i < m; i++){
		matrix->values[i] = (float*)malloc(n * sizeof(float));
		if(matrix->values[i] == NULL) return NULL;
	}
	matrix->n = n;
	matrix->m = m;

	return matrix;
}

int initialize_matrix(Matrix* m, int zeros){
	if(!m) return 0;

	for(size_t i = 0; i < m->m; i++){
		for(size_t j = 0; j < m->n; j++){
			if(!zeros){
				m->values[i][j] = ((rand() ^ i ^ j) % 100);
			}
			else{
				m->values[i][j] = 0;
			}
		}
	}
	return 1;
}

void free_matrix(Matrix** pm){
	if(pm == NULL || *pm == NULL) return;

	for(size_t i = 0; i < (*pm)->m; i++){
		free((*pm)->values[i]);
	}
	free((*pm)->values);
	free(*pm);
	*pm = NULL;
	return;
}


Matrix* matrix_add(Matrix* a, Matrix* b) {
	if(!a || !b) return NULL;
	if(a->m != b->m || a->n != b->n) return NULL;
	
	size_t n = a->n;
	size_t m = a->m;

	Matrix* c = create_matrix(m, n);

	for(size_t i = 0; i < m; i++){
		for(size_t j = 0; j < n; j++){
			c->values[i][j] = a->values[i][j] + b->values[i][j];
		}
	}
	return c;
}

Matrix* matrix_scalar_mul(float scalar, Matrix* m){
	if(m == NULL) return NULL;
	Matrix* sm = create_matrix(m->m, m->n);
	for(size_t i = 0; i < m->m; i++){
		for(size_t j = 0; j < m->n; j++){
			sm->values[i][j] = scalar * m->values[i][j];
		}
	}
	return sm;
}

Matrix* matrix_transpose(Matrix* m){

	Matrix* t = create_matrix(m->n, m->m);
	for(size_t i = 0; i < m->m; i++){
		for(size_t j = 0; j < m->n; j++){
			t->values[j][i] = m->values[i][j];
		}
	}
	return t;
}

Matrix* matrix_mul(Matrix* a, Matrix* b){
	if(!a || !b) return NULL;
	if(a->n != b->m) return NULL;

	Matrix* bt = matrix_transpose(b);
	Matrix* c = create_matrix(a->m, b->n);
	initialize_matrix(c, 1);
	for(size_t i = 0; i < a->m; i++){
		for(size_t j = 0; j < b->n; j++){
			for(size_t k = 0; k < a->n; k++){
				c->values[i][j] += a->values[i][k] * bt->values[j][k];
			}
		}
	}
	return c;
}

void print_matrix(Matrix* m){
	for(size_t i = 0; i < m->m; i++){
		printf("| ");
		for(size_t j = 0; j < m->n; j++){
			printf("%f ", m->values[i][j]);
		}
		printf("|\n");
	}
}

//Parte paralela

void* matrix_mul_worker(void* arg){
	MatrixMulArgs* args = (MatrixMulArgs*)arg;
	Matrix* a = args->a;
	Matrix* b = args->b;
	Matrix* bt = matrix_transpose(b);
	Matrix* c = args->c;
	size_t row_start = args->row_start;
	size_t row_end = args->row_end;

	for(size_t i = row_start; i < row_end; i++){
		for(size_t j = 0; j < b->n; j++){
			for(size_t k = 0; k < a->n; k++){
				c->values[i][j] += a->values[i][k] * bt->values[j][k];
			}
		}
	}

	return NULL;
}

int main() {

	pthread_t threads[NUM_THREADS];
	MatrixMulArgs args[NUM_THREADS];
	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);

	Matrix* a = create_matrix(DIMENSION, DIMENSION);
	Matrix* b = create_matrix(DIMENSION, DIMENSION);
	Matrix* c = create_matrix(DIMENSION, DIMENSION);

	initialize_matrix(a, 0);
	initialize_matrix(b, 0);
	initialize_matrix(c, 1);

	for(size_t i = 0; i < NUM_THREADS; i++){
		args[i].a = a;
		args[i].b = b;
		args[i].c = c;
		args[i].row_start = i * (DIMENSION/NUM_THREADS);
		args[i].row_start = (i + 1) * (DIMENSION/NUM_THREADS);
		pthread_create(&threads[i], NULL, matrix_mul_worker, (void*)&args[i]);
	}

	for(size_t i = 0; i < NUM_THREADS; i++){
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);

	double time = (((t1.tv_sec - t0.tv_sec) * 1000) + ((t1.tv_nsec - t0.tv_nsec) / 1000000));
	
	printf("%f\n", time);

	return 0;
}