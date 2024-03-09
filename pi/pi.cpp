#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <mpi.h>

long double series(uint64_t from, uint64_t to) {
    long double ret = 0;

    for (uint64_t k = from; k < to; k++) {
        long double diff = (long double)1.0 / (1 + 2 * k) * powl(-1.0, k);

        ret += diff;
    }

    return ret;
}

int main(int argc, char **argv) {
    int process_rank, size_of_cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_of_cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    if (argc < 2 && process_rank == 0) {
        printf("USAGE: pi.elf [NUM_OF_ITERATIONS]\n");
        MPI_Finalize();
        return 1;
    }

    uint64_t len = strtoul(argv[1], NULL, 10);

    if (len == 0) {
        printf("Invalid NUM_OF_ITERATIONS\n");
        MPI_Finalize();
        return 1;
    }

    uint64_t chunk_size = len / size_of_cluster;

    printf("%d: Started [%ld; %ld)\n", process_rank, chunk_size * process_rank, chunk_size * (process_rank + 1));
    long double sum = series(chunk_size * process_rank, chunk_size * (process_rank + 1)) * 4;
    
    // MPI_Barrier(MPI_COMM_WORLD);

    long double res;

    if (process_rank < size_of_cluster - 1) {
        MPI_Recv(&res, 1, MPI_LONG_DOUBLE, process_rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    if (process_rank > 0) {
        MPI_Send(&sum, 1, MPI_LONG_DOUBLE, process_rank - 1, 0, MPI_COMM_WORLD);
    }

    sum += res;

    MPI_Barrier(MPI_COMM_WORLD);

    if (process_rank == 0) {
        printf("Pi = %.*Lf\n", LDBL_DECIMAL_DIG, sum);
    }

    MPI_Finalize();

    return 0;
}

