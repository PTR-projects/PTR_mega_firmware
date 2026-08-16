#pragma once


#define MAX_SIZE 4  // Define a maximum size for simplicity

typedef struct {
    int rows;
    int cols;
    float data[MAX_SIZE][MAX_SIZE];
} Matrix;


void fill_matrix(Matrix *m, float value);
Matrix add_matrices(const Matrix *a, const Matrix *b);
Matrix subtract_matrices(const Matrix *a, const Matrix *b);
Matrix multiply_matrices(const Matrix *a, const Matrix *b);
Matrix transpose_matrix(const Matrix *a);
int8_t inverse_matrix(const Matrix *a, Matrix *inverse);