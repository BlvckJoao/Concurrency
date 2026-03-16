/* 
Como uma forma de reduzir overhead por memória uma solução inteligente é implementar uma
matriz como um vetor unidimensional e usar os indices para manipular o acesso, isso reduz eventuais
cache miss por conta dos ponteiros em diferentes posições de memória
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define ACCESS(M, i, j) ((M)->data[(i) * (M)->n + (j)]) //Macro de acesso a elementos da matriz

#define DIMENSION 1024
#define NUM_THREADS 8
#define BLOCK 64

typedef struct matrix {
	float* data;
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
        if(!matrix){ 
                free(matrix);
                return NULL;
        }

	matrix->data = (float*)malloc(m * n * sizeof(float));
	if(matrix->data == NULL) {
		free(matrix);
		return NULL;
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
				ACCESS(m, i, j) = ((rand() ^ i ^ j) % 100);
			}
			else{
				ACCESS(m, i, j) = 0;
			}
		}
	}
	return 1;
}

void free_matrix(Matrix** pm){
	if(pm == NULL || *pm == NULL) return;
	free((*pm)->data);
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
			ACCESS(c, i, j) = ACCESS(a, i, j) + ACCESS(b, i, j);
		}
	}
	return c;
}

Matrix* matrix_scalar_mul(float scalar, Matrix* m){
	if(m == NULL) return NULL;
	Matrix* sm = create_matrix(m->m, m->n);
	for(size_t i = 0; i < m->m; i++){
		for(size_t j = 0; j < m->n; j++){
			ACCESS(sm, i, j) = scalar * ACCESS(m, i, j);
		}
	}
	return sm;
}

Matrix* matrix_transpose(Matrix* m){
	if (!m) return NULL;
	Matrix* t = create_matrix(m->n, m->m);

	for (size_t ii = 0; ii < m->m; ii += BLOCK)
	for (size_t jj = 0; jj < m->n; jj += BLOCK) {

		size_t i_end = ii+BLOCK < m->m ? ii+BLOCK : m->m;
		size_t j_end = jj+BLOCK < m->n ? jj+BLOCK : m->n;

		for (size_t i = ii; i < i_end; i++)
		for (size_t j = jj; j < j_end; j++)
			ACCESS(t, j, i) = ACCESS(m, i, j);
	}
	return t;
}

Matrix* matrix_mul(Matrix* a, Matrix* b){
	if(!a || !b) return NULL;
	if(a->n != b->m) return NULL;

	Matrix* bt = matrix_transpose(b);
	Matrix* c = create_matrix(a->m, b->n);
	initialize_matrix(c, 1);
	

	for(size_t ii = 0; ii < a->m; ii += BLOCK){
	for(size_t jj = 0; jj < b->n; jj += BLOCK){
	for(size_t kk = 0; kk < a->n; kk += BLOCK){

		size_t i_end = ii + BLOCK < a->m ? ii + BLOCK : a->m;
		size_t j_end = jj + BLOCK < b->n ? jj + BLOCK : b->n;
		size_t k_end = kk + BLOCK < a->n ? kk + BLOCK : a->n;


		for(size_t i = ii; i < i_end; i++){
		for(size_t j = jj; j < j_end; j++){
		for(size_t k = kk; k < k_end; k++){
			ACCESS(c, i, j) += ACCESS(a, i, k) * ACCESS(bt, j, k);
		}
		}
	}
	}
	}
	}

	return c;
}


void print_matrix(Matrix* m){
	for(size_t i = 0; i < m->m; i++){
		printf("| ");
		for(size_t j = 0; j < m->n; j++){
			printf("%f ", ACCESS(m, i, j));
		}
		printf("|\n");
	}
}

//Parte paralela

void* matrix_mul_worker(void* arg){
	MatrixMulArgs* args = (MatrixMulArgs*)arg;
	Matrix* a = args->a;
	Matrix* b = args->b;
	Matrix* c = args->c;
	size_t row_start = args->row_start;
	size_t row_end = args->row_end;

	for(size_t i = row_start; i < row_end; i++){
		for(size_t j = 0; j < b->n; j++){
			for(size_t k = 0; k < a->n; k++){
				ACCESS(c, i, j) += ACCESS(a, i, k) * ACCESS(b, j, k);
			}
		}
	}

	return NULL;
}

void* matrix_mul_worker_tilled(void* arg) {

	MatrixMulArgs* args = (MatrixMulArgs*)arg;
	Matrix* a = args->a;
	Matrix* b = args->b;
	Matrix* c = args->c;

	size_t row_start = args->row_start;
	size_t row_end   = args->row_end;
	size_t N = b->m;
	size_t K = a->n;

	for (size_t ii = row_start; ii < row_end; ii += BLOCK)
	for (size_t jj = 0;         jj < N;       jj += BLOCK)
	for (size_t kk = 0;         kk < K;       kk += BLOCK) {

		size_t i_end = ii + BLOCK < row_end ? ii + BLOCK : row_end;
		size_t j_end = jj + BLOCK < N       ? jj + BLOCK : N;
		size_t k_end = kk + BLOCK < K       ? kk + BLOCK : K;

		for (size_t i = ii; i < i_end; i++)
		for (size_t j = jj; j < j_end; j++)
		for (size_t k = kk; k < k_end; k++){
			ACCESS(c, i, j) += ACCESS(a, i, k) * ACCESS(b, j, k);
    	}
	}
	return NULL;
}

int main() {

	pthread_t threads[NUM_THREADS];
	MatrixMulArgs args[NUM_THREADS];
	struct timespec t0, t1;

	Matrix* a = create_matrix(DIMENSION, DIMENSION);
	Matrix* b = create_matrix(DIMENSION, DIMENSION);
	Matrix* c = create_matrix(DIMENSION, DIMENSION);

	initialize_matrix(a, 0);
	initialize_matrix(b, 0);
	initialize_matrix(c, 1);

	Matrix* t = matrix_transpose(b);

	printf("=============TESTE DE MULTIPLICAÇÃO DE MATRIZES=============\n\n");
	printf("Matrizes de dimensão %dx%d\n", DIMENSION, DIMENSION);
	printf("Operação de multiplicação Sequencial vs Paralelo\n\n");

	//teste sequencial
	clock_gettime(CLOCK_MONOTONIC, &t0);
	Matrix* rst = matrix_mul(a, b);
	clock_gettime(CLOCK_MONOTONIC, &t1);
	double time_seq = (((t1.tv_sec - t0.tv_sec) * 1000) + ((t1.tv_nsec - t0.tv_nsec) / 1000000));
	printf("Tempo sequencial: %f ms\n", time_seq);

	//teste paralelo

	clock_gettime(CLOCK_MONOTONIC, &t0);

	for(size_t i = 0; i < NUM_THREADS; i++){
		args[i].a = a;
		args[i].b = t;
		args[i].c = c;
		args[i].row_start = i * (DIMENSION/NUM_THREADS);
		args[i].row_end = (i + 1) * (DIMENSION/NUM_THREADS);
		pthread_create(&threads[i], NULL, matrix_mul_worker_tilled, (void*)&args[i]);
	}

	for(size_t i = 0; i < NUM_THREADS; i++){
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &t1);

	double time_par = (((t1.tv_sec - t0.tv_sec) * 1000) + ((t1.tv_nsec - t0.tv_nsec) / 1000000));
	
	printf("Tempo de execução paralela: %f ms\n", time_par);

	free_matrix(&a);
	free_matrix(&b);
	free_matrix(&c);
	free_matrix(&t);
        free_matrix(&rst);

	return 0;
}