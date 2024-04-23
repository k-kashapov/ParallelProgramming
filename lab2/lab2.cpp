#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#ifdef DBG
    #define DEBUG(...) do { printf("DBG: "   __VA_ARGS__); } while(0)
#else // ifndef DBG 
    #define DEBUG(...) 
#endif // DBG

#ifdef ERR
    #define ERROR(...) do { printf("ERROR: " __VA_ARGS__); } while(0)
#else // ifndef ERR
    #define ERROR(...) 
#endif // ERR


// ========< Misc >========
struct interval {
    long double start_x;
    long double end_x;
    long double fa;
    long double fb;
    long double approx;
};

struct stack {
    struct interval *data;
    int len;
    int cap;
};

int pop(struct stack *stk, struct interval *res) {
    if (stk->len < 1) {
        ERROR("stack underflow\n");
        return -1;
    }

    *res = stk->data[--stk->len];
    return 0;
}

int push(struct stack *stk, struct interval *data) {
    if (stk->len >= stk->cap - 1) {
        void *new_ptr = realloc(stk->data, stk->cap * 2 * sizeof(struct interval));
        if (new_ptr) {
            stk->data = (struct interval *)new_ptr;
            stk->cap *= 2;
        } else {
            ERROR("stack overflow\n");
            return -1;
        }
    }

    stk->data[stk->len++] = *data;
    return 0;
}

int stk_ctor(struct stack *stk, int init_cap) {
    struct interval* data = (struct interval *) calloc(init_cap, sizeof(struct interval));

    if (data == NULL) {
        ERROR("couldn't alloc memory of size: %ld\n", init_cap * sizeof(struct interval));
        return -1;
    }

    stk->data = data;
    stk->cap  = init_cap;

    return 0;
}

void stk_dtor(struct stack *stk) {
    free(stk->data);
    stk->len = 0;
    stk->cap = 0;
}

// ========< Task-specific >=========
struct task {
    long double start_x;
    long double end_x;

    struct stack globl_stk;
    long double sum;
    int nactive;
    int nproc;

    sem_t stk_sem; // Stack access mutex
    sem_t sum_sem; // Sum access mutex
    sem_t tsk_sem; // Tasks present semaphore
};

static struct task integral = {};

// TODO: remove
sem_t test_sem;
int ranks = 0;

long double func(long double x) {
    return sinl(1.0 / x);
}

void *calcIntegral(void *vargp) {
    (void) vargp;
    struct interval my_task;
    const long double eps = 1e-14;
    const int spk = 10;
    long double s = 0;

    struct stack locl_stk = {};

    stk_ctor(&locl_stk, spk);

    // TODO: remove
    sem_wait(&test_sem);
    int rank = ranks++;
    sem_post(&test_sem);

    while (1) {
        DEBUG("%d: New iteration\n", rank);
        // Wait for any tasks in the stack
        sem_wait(&integral.tsk_sem);

        sem_wait(&integral.stk_sem);

        if (integral.globl_stk.len <= 0) {
            sem_post(&integral.stk_sem);
            break;
        }

        pop(&integral.globl_stk, &my_task);

        // If there are unfinished tasks in the global stack
        if (integral.globl_stk.len > 0) {
            sem_post(&integral.tsk_sem);
        }

        if (my_task.start_x <= my_task.end_x) {
            integral.nactive++;
        }

        sem_post(&integral.stk_sem);

        if (my_task.start_x > my_task.end_x) {
            break;
        }

        // Process local stack
        while (1) {
            // Calculate sum
            // DEBUG("Calculating [%Lg, %Lg]\n", my_task.start_x, my_task.end_x);

            long double c = (my_task.start_x + my_task.end_x) / 2.0;
            long double fc = func(c);

            long double sum_ac = (my_task.fa + fc) * (c - my_task.start_x) / 2.0;
            long double sum_cb = (fc + my_task.fb) * (my_task.end_x - c) / 2.0;

            long double sum = sum_ac + sum_cb;
            // DEBUG("sum [%Lg; %Lg) = %Lg\n", my_task.start_x, c, sum_ac);
            // DEBUG("sum [%Lg; %Lg) = %Lg\n", c, my_task.end_x, sum_cb);

            if (fabs(sum - my_task.approx) <= eps) {
                s += sum;

                if (locl_stk.len == 0) break;

                pop(&locl_stk, &my_task);
            } else {
                struct interval new_task = {my_task.start_x, c, my_task.fa, fc, sum_ac};
                push(&locl_stk, &new_task);

                my_task.start_x = c;
                my_task.fa = fc;
                my_task.approx = sum_cb;
            }

            if ((locl_stk.len > integral.nproc) && (integral.globl_stk.len == 0)) {
                sem_wait(&integral.stk_sem);
                DEBUG("%d: Sharing with others\n", rank);

                while (locl_stk.len > 1) {
                    struct interval curr;
                    pop(&locl_stk, &curr);
                    push(&integral.globl_stk, &curr);
                    sem_post(&integral.tsk_sem);
                }

                sem_post(&integral.stk_sem);
            }

            sched_yield();
        }

        DEBUG("%d: Exited local stack\n", rank);

        sem_wait(&integral.stk_sem);
        integral.nactive--;

        if ((integral.nactive == 0) && (integral.globl_stk.len == 0)) {
            printf("%d: Filling with terminating intervals\n", rank);
            for (int i = 0; i < integral.nproc; i++) {
                struct interval curr = {2, 1, 0, 0, 0};
                push(&integral.globl_stk, &curr);
                sem_post(&integral.tsk_sem);
            }
        }

        sem_post(&integral.stk_sem);
    }

    sem_wait(&integral.sum_sem);
    integral.sum += s;
    sem_post(&integral.sum_sem);

    stk_dtor(&locl_stk);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        printf("USAGE: lab2.elf NUM_OF_THREADS START_X END_X\n");
        return 1;
    }

    int size_of_cluster = atoi(argv[1]);
    if (size_of_cluster <= 0) {
        ERROR("Invalid amount of threads specified!\n");
        return 1;
    }

    char *endptr;
    integral.start_x = strtof(argv[2], &endptr);
    if (endptr == argv[2]) {
        ERROR("Invalid START_X specified!\n");
        return 1;
    }

    integral.end_x = strtof(argv[3], &endptr);
    if (endptr == argv[3]) {
        ERROR("Invalid END_X specified!\n");
        return 1;
    }

    printf("Calculating from %Lg to %Lg\n", 
            integral.start_x, integral.end_x);

    int res = stk_ctor(&integral.globl_stk, size_of_cluster);

    if (res) {
        return -1;
    }

    integral.nproc = size_of_cluster;

    struct interval curr = {integral.start_x, integral.end_x, func(curr.start_x), func(curr.end_x), 0};
    curr.approx = (curr.fa + curr.fb) * (curr.end_x - curr.start_x) / 2.0;

    DEBUG("init interval = {%Lg, %Lg, %Lg, %Lg, %Lg}\n", curr.start_x, curr.end_x, curr.fa, curr.fb, curr.approx);

    push(&integral.globl_stk, &curr);

    sem_init(&integral.sum_sem, 0, 1);
    sem_init(&integral.stk_sem, 0, 1);
    sem_init(&integral.tsk_sem, 0, 1);

    // TODO: remove
    sem_init(&test_sem, 0, 1);

    pthread_t *thread_ids = (pthread_t *) calloc(size_of_cluster, sizeof(pthread_t));

    for (int i = 0; i < size_of_cluster; i++) {
        pthread_create(thread_ids + i, NULL, calcIntegral, NULL);
        DEBUG("Started thread %d\n", i);
    }

    for (int i = 0; i < size_of_cluster; i++) {
        pthread_join(thread_ids[i], NULL);
        DEBUG("Thread %d finished\n", i);
    }

    printf("Resulting sum = %.10Lg\n", integral.sum);

    free(thread_ids);

    stk_dtor(&integral.globl_stk);
    sem_destroy(&integral.sum_sem);
    sem_destroy(&integral.stk_sem);
    sem_destroy(&integral.tsk_sem);

    // TODO: remove
    sem_destroy(&test_sem);

    return 0;
}
