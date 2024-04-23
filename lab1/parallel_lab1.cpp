#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <raylib.h>
#include <mpi.h>
#include <assert.h>

typedef struct dim {
    double len;
    uint32_t pts_num;
    double step;
} dim;

typedef struct dimensions {
    dim x;
    dim t;
} dimensions;

typedef struct data_xy {
    double **data;
    dimensions d;
    double d_coeff;
} data_xy;

static double max_abs(data_xy *state) {
    double res = 0.0;

    for (uint32_t i = 0; i < state->d.x.pts_num; i++) {
        for (uint32_t j = 0; j < state->d.t.pts_num; j++) {
            double abs_data = abs(state->data[i][j]);
            if (abs_data > res) {
                res = abs_data;
            }
        }
    }

    return res ? res : 1.0;
}

static void draw_results(data_xy *state) {
    const double scale_x = 1000.0 / state->d.t.pts_num;
    const double scale_y = 600.0 / state->d.x.pts_num;
    const double padding = 10;

    InitWindow((state->d.t.pts_num + padding) * scale_x, (state->d.x.pts_num + padding) * scale_y, "Temperature");
    SetTargetFPS(1);

    double max_val = max_abs(state);

    uint32_t step_x = (uint32_t)(1.0/scale_x);
    uint32_t step_y = (uint32_t)(1.0/scale_y);

    printf("max_val = %lf, step_x = %d, step_y = %d\n", max_val, step_x, step_y);

    step_x = step_x == 0 ? 1 : step_x;
    step_y = step_y == 0 ? 1 : step_y;

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);

        for (uint32_t j = padding / 2; j < state->d.x.pts_num / step_y; j++) {
            for (uint32_t i = padding / 2; i < state->d.t.pts_num / step_x; i++) {
                double normalized = state->data[j * step_y][i * step_x] / max_val / 2;
                double hue = normalized * 180 + 180;

                DrawPixel(i, j, ColorFromHSV(hue, 1.0, 1.0));
            }
        }

        EndDrawing();
    }

    CloseWindow();
}

static double **data_alloc(uint32_t pts_x, uint32_t pts_y) {
    double **data = (double **) calloc(pts_x, sizeof(double *));
    assert(data);

    double *data_buf = (double *) calloc(pts_x * pts_y, sizeof(double));
    assert(data_buf);

    for (uint32_t i = 0; i < pts_x; i++) {
        data[i] = data_buf + i * pts_y;
    }

    return data;
}

static void data_print(data_xy *state) {
    for (uint32_t i = 0; i < state->d.x.pts_num; i++) {
        for (uint32_t j = 0; j < state->d.t.pts_num; j++) {
            printf("%g ", state->data[i][j]);
        }
        putc('\n', stdout);
    }
}

static double calculate_new_point(data_xy *state, uint32_t x_pos, uint32_t y_pos) {
    double **data = state->data;

    double r_side = (data[x_pos + 1][y_pos] - 2 * data[x_pos][y_pos] + data[x_pos - 1][y_pos]) / state->d.x.step / state->d.x.step;

    return data[x_pos][y_pos] + state->d.t.step * state->d_coeff * r_side;
}

static void calculate_row(data_xy *state, uint32_t row, uint32_t from, uint32_t to, uint32_t rank, uint32_t size) {
    for (uint32_t i = from; i < to; i++) {
        state->data[i][row] = calculate_new_point(state, i, row - 1);
    }

    if (rank > 0) {
        MPI_Recv(&state->data[from - 1][row], 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(&state->data[from][row],     1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD);
    }

    if (rank < size - 1) {
        MPI_Send(&state->data[to - 1][row], 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);
        MPI_Recv(&state->data[to][row],     1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

static void data_free(data_xy *state) {
    free(state->data[0]);
    free(state->data);
}

int main(int argc, char **argv) {
    int process_rank, size_of_cluster;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size_of_cluster);
    MPI_Comm_rank(MPI_COMM_WORLD, &process_rank);

    dim x = {100.0, 100000, 0.0};
    x.step = x.len / x.pts_num;

    dim t = {0.01, 2000, 0.0};
    t.step = t.len / t.pts_num;

    const dimensions dims = {x, t};

    double **data = data_alloc(x.pts_num, t.pts_num);

    for (uint32_t i = 0; i < x.pts_num; i++) {
        data[i][0] = sin(x.step * i * 5);
    }

    for (uint32_t i = 0; i < t.pts_num; i++) {
        data[0][i] = data[0][0] + (t.step * i) * 5;
    }

    for (uint32_t i = 1; i < t.pts_num; i++) {
        data[x.pts_num - 1][i] = data[x.pts_num - 1][0] - (t.step * i) * 4;
    }

    data_xy state = {data, dims, 5e-2};

    assert(state.d_coeff * t.step / x.step / x.step < 1);

    // MPI_Barrier(MPI_COMM_WORLD);

    uint32_t chunk_size = (x.pts_num / (size_of_cluster));
    
    uint32_t init_i = 1 + chunk_size * process_rank;
    uint32_t end_i = init_i + chunk_size;

    if ((process_rank == size_of_cluster - 1) || (end_i > x.pts_num - 1)) {
        end_i = x.pts_num - 1;
    }

    printf("Process %d started [%d; %d]\n", process_rank, init_i, end_i - 1);

    double start = MPI_Wtime();

    for (uint32_t row = 1; row < t.pts_num; row++) {
        calculate_row(&state, row, init_i, end_i, process_rank, size_of_cluster);
    }

    double end = MPI_Wtime();

    printf("Process %d finished, time = %lf\n", process_rank, end - start);

    // char name[] = "logs/process_log_        ";
    // sprintf(name + 17, "%d_of_%d", process_rank, size_of_cluster);

    // FILE *log = fopen(name, "w");

    // for (uint32_t i = init_i; i < end_i; i++) {
    //     fprintf(log, "data[%d][%d] = %lf\n", i, t.pts_num - 1, data[i][t.pts_num - 1]);
    // }

    // fclose(log);

    MPI_Finalize();
    draw_results(&state);
    data_free(&state);

    return 0;
}
