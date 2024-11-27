#include <assert.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void usage(const char *name) {
    printf("USAGE: %s ISIZE JSIZE OUTPUT_FILE\n", name);
}

#define at(i, j) (buf[(i) * JSIZE + (j)])
#define REPEAT_NUM 300

int main(int argc, char **argv) {
    int ISIZE = 0;
    int JSIZE = 0;
    double *buf = NULL;

    if (argc < 4) {
        usage(argv[0]);
        return 0;
    }

    char *arg_end = NULL;
    ISIZE = strtol(argv[1], &arg_end, 10);
    if (arg_end == argv[1]) {
        printf("ERROR: invalid ISIZE\n");
        usage(argv[0]);
        return 0;
    }

    arg_end = NULL;
    JSIZE = strtol(argv[2], &arg_end, 10);
    if (arg_end == argv[2]) {
        printf("ERROR: invalid JSIZE\n");
        usage(argv[0]);
        return 0;
    }

    int leftover_size = 3 * JSIZE * sizeof(double);
    buf = (double *)malloc(ISIZE * JSIZE * sizeof(double) + leftover_size);
    if (buf == NULL) {
        printf("ERROR: Could not allocate buffer\n");
        exit(1);
    }

    #pragma omp for schedule(static)
    for (int i = 0; i < ISIZE; i++) {
        for (int j = 0; j < JSIZE; j++) {
            at(i, j) = 10 * i + j;
        }
    }

    double *leftover = buf + ISIZE * JSIZE;

    // Save rightmost 3 columns to additional space in bufer
    memcpy(leftover, buf + (ISIZE - 3) * JSIZE, leftover_size);

    double start = omp_get_wtime();

    for (int i = 0; i < REPEAT_NUM; i++) {
        #pragma omp parallel
        {
            #pragma omp for schedule(auto)
            for (int i = 0; i < ISIZE; i++) {
                for (int j = 0; j < JSIZE; j++) {
                    at(i, j) = 10 * i + j;
                }
            }

            #pragma omp for schedule(auto)
            for (int i = 3; i < ISIZE; i++) {
                for (int j = 2; j < JSIZE; j++) {
                    at(i, j) = sin(0.1 * at(i, j - 2));
                }
            }
        }

        for (int i = 3; i < ISIZE; i++) {
            at(i, 0) = at(i - 3, 0);
            at(i, 1) = at(i - 3, 0);
        }
    }

    double end = omp_get_wtime();

    double elapsed = (float)(end - start);

    printf("Elapsed: %lf\n"
           "One cycle = %lfs\n",
           elapsed, elapsed / REPEAT_NUM);

    FILE *ff = fopen(argv[3], "w");

    for (int i = 3; i < ISIZE + 3; i++) {
        for (int j = 0; j < JSIZE; j++) {
            fprintf(ff, "%f ", at(i, j));
        }

        fprintf(ff, "\n");
    }

    fclose(ff);
    free(buf);
    return 0;
}