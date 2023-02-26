#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "rtos-alloc.h"

#define NUM_ALLOCS 100000
#define MAX_ALLOC_SIZE 16384

int main() {
    struct timespec start, end;
    double time_used;

    // Allocate and free memory using libc's malloc(3)
    for (size_t size = 1; size <= MAX_ALLOC_SIZE; size *= 2) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < NUM_ALLOCS; i++) {
            void *ptr = malloc(size);
            free(ptr);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_used = (end.tv_sec - start.tv_sec) + (double) (end.tv_nsec - start.tv_nsec) / 1000000000;
        printf("libc, %zu, %f\n", size, time_used);
    }

    // Allocate and free memory using your allocator
    for (size_t size = 1; size <= MAX_ALLOC_SIZE; size *= 2) {
        clock_gettime(CLOCK_MONOTONIC, &start);
        for (int i = 0; i < NUM_ALLOCS; i++) {
            void *ptr = rtos_malloc(size);
            rtos_free(ptr);
        }
        clock_gettime(CLOCK_MONOTONIC, &end);
        time_used = (end.tv_sec - start.tv_sec) + (double) (end.tv_nsec - start.tv_nsec) / 1000000000;
        printf("rtos, %zu, %f\n", size, time_used);
    }

    return 0;
}
