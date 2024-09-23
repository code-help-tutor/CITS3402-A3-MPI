WeChat: cstutorcs
QQ: 749389476
Email: tutorcs@163.com
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

// Function to read the sparse matrix from a file in matrix market format
void readSparseMatrix(const char *filename, int **row, int **col, float **val, int *numRows, int *numCols, int *numNonZero) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    fscanf(file, "%*[^\n]\n"); // Skip the header line

    int r, c;
    float v;
    *numNonZero = 0;
    while (fscanf(file, "%d %d %f\n", &r, &c, &v) == 3) {
        (*numNonZero)++;
    }

    rewind(file);
    fscanf(file, "%*[^\n]\n"); // Skip the header line

    *row = (int *)malloc(*numNonZero * sizeof(int));
    *col = (int *)malloc(*numNonZero * sizeof(int));
    *val = (float *)malloc(*numNonZero * sizeof(float));

    int i = 0;
    while (fscanf(file, "%d %d %f\n", &r, &c, &v) == 3) {
        (*row)[i] = r;
        (*col)[i] = c;
        (*val)[i] = v;
        i++;
    }

    fclose(file);

    *numRows = r;
    *numCols = c;
}

// Function to perform matrix-matrix multiplication
void matrixMatrixMultiplication(int *rowA, int *colA, float *valA, int numRowsA, int numColsA, int numNonZeroA,
                                int *rowB, int *colB, float *valB, int numRowsB, int numColsB, int numNonZeroB,
                                float **result, int *numRowsResult, int *numColsResult) {
    *numRowsResult = numRowsA;
    *numColsResult = numColsB;

    *result = (float *)malloc(numRowsA * numColsB * sizeof(float));
    for (int i = 0; i < numRowsA * numColsB; i++) {
        (*result)[i] = 0.0;
    }

#pragma omp parallel for shared(rowA, colA, valA, rowB, colB, valB, result)
    for (int i = 0; i < numNonZeroA; i++) {
        int row = rowA[i];
        int col = colA[i];
        float val = valA[i];

        for (int j = 0; j < numNonZeroB; j++) {
            if (colB[j] == col) {
                int resultRow = row;
                int resultCol = colB[j];
#pragma omp atomic
                (*result)[resultRow * numColsB + resultCol] += val * valB[j];
            }
        }
    }
}

// Function to distribute the matrices to other nodes
void distributeMatrices(int rank, int size, int *rowA, int *colA, float *valA, int numRowsA, int numColsA, int numNonZeroA,
                        int *rowB, int *colB, float *valB, int numRowsB, int numColsB, int numNonZeroB) {
    if (rank == 0) {
        // Send matrix A to other nodes
        for (int i = 1; i < size; i++) {
            MPI_Send(&numRowsA, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&numColsA, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&numNonZeroA, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(rowA, numNonZeroA, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(colA, numNonZeroA, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(valA, numNonZeroA, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
        }

        // Send matrix B to other nodes
        for (int i = 1; i < size; i++) {
            MPI_Send(&numRowsB, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&numColsB, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(&numNonZeroB, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(rowB, numNonZeroB, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(colB, numNonZeroB, MPI_INT, i, 0, MPI_COMM_WORLD);
            MPI_Send(valB, numNonZeroB, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
        }
    } else {
        // Receive matrix A from the master node
        MPI_Recv(&numRowsA, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&numColsA, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&numNonZeroA, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        rowA = (int *)malloc(numNonZeroA * sizeof(int));
        colA = (int *)malloc(numNonZeroA * sizeof(int));
        valA = (float *)malloc(numNonZeroA * sizeof(float));
        MPI_Recv(rowA, numNonZeroA, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(colA, numNonZeroA, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(valA, numNonZeroA, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Receive matrix B from the master node
        MPI_Recv(&numRowsB, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&numColsB, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(&numNonZeroB, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        rowB = (int *)malloc(numNonZeroB * sizeof(int));
        colB = (int *)malloc(numNonZeroB * sizeof(int));
        valB = (float *)malloc(numNonZeroB * sizeof(float));
        MPI_Recv(rowB, numNonZeroB, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(colB, numNonZeroB, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(valB, numNonZeroB, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *rowA, *colA, *valA;
    int numRowsA, numColsA, numNonZeroA;

    int *rowB, *colB, *valB;
    int numRowsB, numColsB, numNonZeroB;

    // Read matrix A
    if (rank == 0) {
        readSparseMatrix("matrixA.mtx", &rowA, &colA, &valA, &numRowsA, &numColsA, &numNonZeroA);
    }

    // Read matrix B
    if (rank == 0) {
        readSparseMatrix("matrixB.mtx", &rowB, &colB, &valB, &numRowsB, &numColsB, &numNonZeroB);
    }

    // Distribute the matrices to other nodes
    distributeMatrices(rank, size, rowA, colA, valA, numRowsA, numColsA, numNonZeroA,
                       rowB, colB, valB, numRowsB, numColsB, numNonZeroB);

    float *result;
    int numRowsResult, numColsResult;

    // Perform matrix-matrix multiplication
    matrixMatrixMultiplication(rowA, colA, valA, numRowsA, numColsA, numNonZeroA,
                               rowB, colB, valB, numRowsB, numColsB, numNonZeroB,
                               &result, &numRowsResult, &numColsResult);

    // Print the result (for demonstration purposes)
    if (rank == 0) {
        printf("Result matrix:\n");
        for (int i = 0; i < numRowsResult; i++) {
            for (int j = 0; j < numColsResult; j++) {
                printf("%f ", result[i * numColsResult + j]);
            }
            printf("\n");
        }
    }

    MPI_Finalize();

    return 0;
}
