#include <mpi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
    int process_rank, size_of_cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_of_cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    if (size_of_cluster != 2 || argc < 3) {
        printf("USAGE: mpirun -np 2 msg.elf [NUM_OF_ITERATIONS] [BUFFER_LEN]\n");
        MPI_Finalize();
        return 1;
    }

    uint64_t count = strtoul(argv[1], NULL, 10);

    if (count == 0) {
        printf("Invalid NUM_OF_ITERATIONS\n");
        MPI_Finalize();
        return 1;
    }

    uint64_t buf_len = strtoul(argv[2], NULL, 10);

    if (buf_len == 0) {
        printf("Invalid BUFFER_LEN\n");
        MPI_Finalize();
        return 1;
    }

    int *buf = (int *) malloc (buf_len * sizeof(int));

    // printf("%d: Start\n", process_rank);
    uint64_t delta_ns = 0;

    for (unsigned len = 0; len < buf_len; len++) {
        printf("%d ", len);
        for (unsigned rep = 0; rep < 4; rep++) {
            if (process_rank == 0) {
                struct timespec start, end;
                clock_gettime(CLOCK_MONOTONIC_RAW, &start);

                for (uint64_t i = 0; i < count; i++) {
                    MPI_Recv(buf, len, MPI_INT, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(buf, len, MPI_INT, 1, 0, MPI_COMM_WORLD);
                    // printf("\rRecv: %lu/%lu", i, count);
                }

                clock_gettime(CLOCK_MONOTONIC_RAW, &end);

                delta_ns = (end.tv_sec - start.tv_sec) * 1'000'000'000 + (end.tv_nsec - start.tv_nsec);
                // printf("Overall time: %lu ns\n", delta_ns);
                printf("%lu ", delta_ns / count / 2);
            } else {
                for (uint64_t i = 0; i < count; i++) {
                    MPI_Send(buf, len, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    MPI_Recv(buf, len, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                }
            }
        }
        printf("\n");
    }

    MPI_Finalize();

    // printf("\n%d: Finish\n", process_rank);

    return 0;
}
