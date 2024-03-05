#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <malloc.h>
#include <raylib.h>

typedef struct dim {
    float len;
    uint32_t pts_num;
    float step;
} dim;

typedef struct dimensions {
    dim x;
    dim t;
} dimensions;

typedef struct data_xy {
    float **data;
    dimensions d;
    float d_coeff;
} data_xy;

static float max_abs(data_xy *state) {
    float res = 0.0;

    for (uint32_t i = 0; i < state->d.x.pts_num; i++) {
        for (uint32_t j = 0; j < state->d.t.pts_num; j++) {
            float abs_data = abs(state->data[i][j]);
            if (abs_data > res) {
                res = abs_data;
            }
        }
    }

    return res ? res : 1.0;
}

static void draw_results(data_xy *state) {
    const float scale_x = 1000.0 / state->d.t.pts_num;
    const float scale_y = 600.0 / state->d.x.pts_num;
    const float padding = 10;

    InitWindow((state->d.t.pts_num + padding) * scale_x, (state->d.x.pts_num + padding) * scale_y, "Temperature");
    SetTargetFPS(5);

    float max_val = max_abs(state);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);

        uint32_t last_pixel_x = 0;
        uint32_t last_pixel_y = 0;

        for (uint32_t i = 0; i < state->d.t.pts_num; i++) {
            for (uint32_t j = 0; j < state->d.x.pts_num; j++) {
                uint32_t pixel_x = (i + padding / 2) * scale_x;
                uint32_t pixel_y = (j + padding / 2) * scale_y;

                if (pixel_x == last_pixel_x && pixel_y == last_pixel_y) continue;

                last_pixel_x = pixel_x;
                last_pixel_y = pixel_y;

                float normalized = state->data[j][i] / max_val / 2;
                float hue = normalized * 180 + 180;

                int pixel_size_x = scale_x <= 1 ? 1 : (int) scale_x;
                int pixel_size_y = scale_y <= 1 ? 1 : (int) scale_y;

                DrawRectangle(pixel_x, pixel_y, pixel_size_x, pixel_size_y, ColorFromHSV(hue, 1.0, 1.0));
            }
        }

        EndDrawing();
    }

    CloseWindow();
}

static float **data_alloc(uint32_t pts_x, uint32_t pts_y) {
    float **data = (float **) calloc(pts_x, sizeof(data[0]));
    
    for (uint32_t i = 0; i < pts_x; i++) {
        data[i] = (float *) calloc(pts_y, sizeof(data[i][0]));
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

static float calculate_new_point(data_xy *state, uint32_t x_pos, uint32_t y_pos) {
    float **data = state->data;

    float r_side = (data[x_pos + 1][y_pos] - 2 * data[x_pos][y_pos] + data[x_pos - 1][y_pos]) / state->d.x.step / state->d.x.step;

    return data[x_pos][y_pos] + state->d.t.step * state->d_coeff * r_side;
}

static void calculate_row(data_xy *state, uint32_t row) {
    if ((row < 1) || (row >= state->d.t.pts_num)) return;
    for (uint32_t i = 1; i < state->d.x.pts_num - 1; i++) {
        state->data[i][row] = calculate_new_point(state, i, row - 1);
    }
}

static void data_free(data_xy *state) {
    for (uint32_t i = 0; i < state->d.x.pts_num; i++) {
        free(state->data[i]);
    }

    free(state->data);
}

int main(int argc, const char **argv) {
    dim x = {1.0, 1000, 0.0};
    x.step = x.len / x.pts_num;

    dim t = {10.0, 20000, 0.0};
    t.step = t.len / t.pts_num;

    const dimensions dims = {x, t};

    float **data = data_alloc(x.pts_num, t.pts_num);

    for (uint32_t i = 0; i < x.pts_num; i++) {
        data[i][0] = sin((float)i/x.pts_num) * 100;
    }

    for (uint32_t i = 0; i < t.pts_num; i++) {
        data[0][i] = data[0][0] + ((float)i/100);
    }

    for (uint32_t i = 1; i < t.pts_num; i++) {
        data[x.pts_num - 1][i] = data[x.pts_num - 1][0] + ((float)i/100);
    }

    data_xy state = {data, dims, 1e-5};

    for (uint32_t row = 0; row < t.pts_num; row++) {
        calculate_row(&state, row);
        printf("\rDone %6d/%6d rows", row, t.pts_num);
    }

    printf("\n");

    // data_print(&state);

    draw_results(&state);

    data_free(&state);

    return 0;
}
