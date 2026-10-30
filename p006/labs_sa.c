#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define N 100

static int s[N];
static long C[N];   /* C[k] for k=0..N-1 ; C[0] unused */
static long E;

/* Full recompute of C (k=1..N-1) and energy E = sum C[k]^2 */
static void compute_C_E(void) {
    E = 0;
    for (int k = 1; k < N; k++) {
        long c = 0;
        for (int i = 0; i + k < N; i++) c += (long)s[i] * s[i + k];
        C[k] = c;
        E += c * c;
    }
}

/* Delta in E if we flip s[i]. Does NOT modify state. */
static long delta_flip(int i) {
    int old = s[i];
    long delta = 0;
    for (int k = 1; k < N; k++) {
        long dC = 0;
        if (i + k < N) dC -= 2LL * old * s[i + k];
        if (i - k >= 0) dC -= 2LL * old * s[i - k];
        if (dC != 0) {
            long Ck = C[k];
            delta += 2LL * Ck * dC + dC * dC;
        }
    }
    return delta;
}

/* Apply the flip to s[i] (must match a preceding delta_flip). */
static void apply_flip(int i) {
    int old = s[i];
    for (int k = 1; k < N; k++) {
        long dC = 0;
        if (i + k < N) dC -= 2LL * old * s[i + k];
        if (i - k >= 0) dC -= 2LL * old * s[i - k];
        if (dC != 0) C[k] += dC;
    }
    s[i] = -old;
}

static int best_s[N];
static long bestE = 9223372036854775807LL;

static void save_best(void) {
    if (E < bestE) {
        bestE = E;
        memcpy(best_s, s, sizeof(s));
    }
}

static void copy_best_to_s(void) {
    memcpy(s, best_s, sizeof(s));
    compute_C_E();
}

static void random_start(void) {
    for (int i = 0; i < N; i++) s[i] = (rand() & 1) ? 1 : -1;
    compute_C_E();
    save_best();
}

/* deterministic start from file of '+'/'-' (100 chars) */
static int file_start(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char buf[N + 1];
    if (fread(buf, 1, N, f) != N) { fclose(f); return 0; }
    fclose(f);
    for (int i = 0; i < N; i++) s[i] = (buf[i] == '+') ? 1 : -1;
    compute_C_E();
    save_best();
    return 1;
}

static void perturb(int p) {
    for (int t = 0; t < p; t++) {
        int i = rand() % N;
        long d = delta_flip(i);
        apply_flip(i);
        E += d;
    }
}

int main(int argc, char **argv) {
    int seconds = (argc > 1) ? atoi(argv[1]) : 30;
    unsigned int seed = (argc > 2) ? (unsigned int)atoi(argv[2]) : (unsigned int)(time(0) ^ clock());
    const char *startfile = (argc > 3) ? argv[3] : NULL;
    const char *outfile = (argc > 4) ? argv[4] : "best.txt";
    int perturb_bits = (argc > 5) ? atoi(argv[5]) : 12;

    srand(seed);

    int have = 0;
    if (startfile) have = file_start(startfile);
    if (!have) random_start();
    printf("initial E = %ld\n", E);

    double T0 = 800.0;
    double decay = 0.97;
    int epochs = 400;
    long steps_per_epoch = 4000;

    clock_t t0 = clock();
    double elapsed = 0;
    int restart = 0;
    while (elapsed < seconds) {
        restart++;
        /* start this restart from the global best, perturbed */
        copy_best_to_s();
        perturb(perturb_bits);
        double T = T0;
        for (int ep = 0; ep < epochs && elapsed < seconds; ep++) {
            for (long st = 0; st < steps_per_epoch; st++) {
                int i = rand() % N;
                long d = delta_flip(i);
                if (d <= 0 || drand48() < exp(-(double)d / T)) {
                    apply_flip(i);
                    E += d;
                    if (E < bestE) save_best();
                }
            }
            T *= decay;
            elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
        }
        elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
    }

    printf("restarts = %d  bestE = %ld\n", restart, bestE);
    FILE *o = fopen(outfile, "w");
    if (o) {
        for (int i = 0; i < N; i++) fputc(best_s[i] == 1 ? '+' : '-', o);
        fputc('\n', o);
        fclose(o);
    }
    return 0;
}
