
#include <stdio.h>
#include <time.h>

#define TOTAL_ITERS  1000000000LL
#define PRINT_EVERY  1000000LL

int main(void)
{
    long long iter;
    clock_t start, end;

    start = clock();

    for (iter = 1; iter <= TOTAL_ITERS; iter++) {
        if (iter % PRINT_EVERY == 0) {
            printf(".");
            fflush(stdout);
        }
    }

    end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n1e9 iteraciones completadas.\n");
    printf("Tiempo total: %.4f segundos\n", elapsed);

    return 0;
}
