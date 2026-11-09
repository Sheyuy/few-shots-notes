/*
 * ganet.c -- genetic search for a 17-input sorting network with FEWER
 * comparators than the current best (71).
 *
 * Why a different representation
 * ------------------------------
 * The earlier search (sortnet.c) worked on a FLAT comparator sequence with
 * "delete one comparator, then repair by hill climbing".  After ~1e8
 * evaluations that never moved off 71: the flat representation has no notion
 * of parallelism, so every repair has to re-establish the whole ordering.
 *
 * A sorting network is naturally LAYERED: within one layer the comparators
 * touch disjoint wires and therefore commute.  A GA over layers can move
 * comparators between layers, change their wires, and recombine two
 * networks layer-wise -- operations that are impossible (or hopeless) on a
 * flat sequence.
 *
 * Fitness
 * -------
 *   if the network sorts every input:  fitness = 1e12 + (MAXC - n)
 *                                     (so fewer comparators always wins)
 *   otherwise:                          fitness = (#sorted inputs)
 *
 * Evaluation is the bit-sliced one from sortnet.c: 17 uint64 words, one
 * comparator = 2 ANDs + 2 ORs, 64 inputs at a time -> ~0.6 ms for all
 * 131072 inputs, i.e. ~1600 full evaluations per second.
 *
 * Usage:
 *   ganet <baseline.json> <seconds> <seed> <out.json> [population] [maxC]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define NCH 17
#define MAXC 256
#define MAXL 64
#define TOTAL (1L << NCH)

typedef struct {
    int L;                 /* layers                                   */
    int start[MAXL + 1];   /* comparators of layer t are c[start[t]..] */
    int c[MAXC][2];
    int n;                 /* total comparators                        */
} Net;

/* ---- bit-sliced evaluation (same trick as sortnet.c) ------------------- */
static uint64_t wire[NCH];

static long evaluate(const Net *net)
{
    long good = 0;
    uint64_t pats[64];
    for (long base = 0; base < TOTAL; base += 64) {
        for (int k = 0; k < 64; k++) pats[k] = (uint64_t)(base + k);
        for (int i = 0; i < NCH; i++) {
            uint64_t w = 0;
            if (i < 6) {
                for (int k = 0; k < 64; k++)
                    if ((pats[k] >> i) & 1ULL) w |= 1ULL << k;
            } else {
                if ((base >> i) & 1ULL) w = ~0ULL; else w = 0ULL;
            }
            wire[i] = w;
        }
        /* LAYER BY LAYER: comparators inside one layer touch disjoint wires
         * and therefore commute, so reordering inside a layer is safe -- but
         * the flat order across layers is NOT, so the layers must be executed
         * in order.  Evaluating flat c[0..n) was wrong for this
         * representation and silently changed the network. */
        for (int t = 0; t < net->L; t++) {
            for (int i = net->start[t]; i < net->start[t + 1]; i++) {
                int a = net->c[i][0], b = net->c[i][1];
                uint64_t x = wire[a], y = wire[b];
                wire[a] = x & y;
                wire[b] = x | y;
            }
        }
        uint64_t unsorted = 0;
        for (int i = 0; i + 1 < NCH; i++)
            unsorted |= wire[i] & ~wire[i + 1];
        good += 64 - __builtin_popcountll(unsorted);
    }
    return good;
}

/* ---- rng --------------------------------------------------------------- */
static uint64_t rs = 0x243F6A8885A308D3ULL;
static inline uint64_t rnd64(void) { rs ^= rs << 13; rs ^= rs << 7; rs ^= rs << 17; return rs; }
static inline int rnd(int k) { return (int)(rnd64() % (uint64_t)(k > 0 ? k : 1)); }
static double wall(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

/* ---- layer helpers ----------------------------------------------------- */
/* Insert comparator (a,b) at flat position p, shifting the tail right.
 * The DATA shift is the essential part: shifting only the start[] table
 * silently destroys the comparator that lived at p (the first comparator of
 * the next layer) -- that is what corrupted the networks and produced the
 * garbage wire indices ASan reported. */
static int net_insert_at(Net *n, int p, int a, int b, int maxC)
{
    if (n->n >= maxC || n->n >= MAXC) return 0;
    if (p > n->n) p = n->n;
    if (p >= MAXC) return 0;
    for (int i = n->n; i > p; i--) {
        n->c[i][0] = n->c[i - 1][0];
        n->c[i][1] = n->c[i - 1][1];
    }
    n->c[p][0] = a; n->c[p][1] = b;
    n->n++;
    return 1;
}

/* Append comparator (a,b) as the LAST comparator of layer t.
 * Returns 0 when the network is full (keeps the net consistent). */
static int net_append(Net *n, int t, int a, int b, int maxC)
{
    if (t < 0 || t >= MAXL) return 0;
    int p = n->start[t + 1];                 /* end of layer t */
    if (!net_insert_at(n, p, a, b, maxC)) return 0;
    for (int l = t + 1; l <= n->L; l++) n->start[l]++;
    return 1;
}

/* remove a comparator from layer t */
static void layer_remove(Net *n, int t, int idx)
{
    int p = n->start[t] + idx;
    for (int i = p; i + 1 < n->start[t + 1]; i++) {
        n->c[i][0] = n->c[i + 1][0];
        n->c[i][1] = n->c[i + 1][1];
    }
    for (int l = t + 1; l <= n->L; l++) n->start[l]--;
    n->n--;
}

/* does wire w appear in layer t? */
static int layer_uses(const Net *n, int t, int w)
{
    for (int i = n->start[t]; i < n->start[t + 1]; i++)
        if (n->c[i][0] == w || n->c[i][1] == w) return 1;
    return 0;
}

/* try to put comparator (a,b) into layer t; if the wires are busy, try to
 * evict one of the conflicting comparators (which is then re-inserted into a
 * later layer).  Returns 1 on success. */
static int layer_place(Net *n, int t, int a, int b, int maxC)
{
    if (t >= MAXL) return 0;
    /* NOTE: indices handed to layer_remove() must be RELATIVE to the layer
     * (it adds start[t] itself).  We used to store the absolute index here,
     * which made the eviction remove the wrong comparator and shift c[]
     * across layer boundaries -- that is what produced garbage wire indices
     * (ASan: wire[a] with a = -563028552) and the SEGV in evaluate(). */
    int busyA = -1, busyB = -1;
    for (int i = n->start[t]; i < n->start[t + 1]; i++) {
        int rel = i - n->start[t];
        if (n->c[i][0] == a || n->c[i][1] == a) busyA = rel;
        if (n->c[i][0] == b || n->c[i][1] == b) busyB = rel;
    }
    if (busyA >= 0 || busyB >= 0) {
        if (n->n >= maxC) return 0;
        /* evict the conflicting comparators and re-place them later */
        int evict[2], nev = 0;
        if (busyA >= 0) evict[nev++] = busyA;
        if (busyB >= 0 && busyB != busyA) evict[nev++] = busyB;
        int saved[2][2];
        for (int i = 0; i < nev; i++) {
            saved[i][0] = n->c[n->start[t] + evict[i]][0];
            saved[i][1] = n->c[n->start[t] + evict[i]][1];
        }
        for (int i = nev - 1; i >= 0; i--) layer_remove(n, t, evict[i]);
        /* append (a,b) to layer t */
        if (!net_append(n, t, a, b, maxC)) return 0;
        /* re-place the evicted ones in later layers */
        for (int i = 0; i < nev; i++) {
            int placed = 0;
            for (int l = t + 1; l < n->L && !placed; l++) {
                if (!layer_uses(n, l, saved[i][0]) && !layer_uses(n, l, saved[i][1]))
                    if (net_append(n, l, saved[i][0], saved[i][1], maxC)) placed = 1;
            }
            if (!placed) {
                /* give up on it: keep the net, but do NOT corrupt the last
                 * layer -- start a fresh layer instead (the old code wrote at
                 * start[n->L] and destroyed that layer's first comparator). */
                if (n->n >= maxC || n->n >= MAXC) return 1;
                if (n->L + 1 >= MAXL) return 1;
                n->L++;
                n->c[n->n][0] = saved[i][0]; n->c[n->n][1] = saved[i][1];
                n->n++;
                n->start[n->L] = n->n;
            }
        }
        return 1;
    }
    return net_append(n, t, a, b, maxC);
}

/* Order-preserving greedy parallelisation of a flat comparator sequence.
 * Pass 1 decides the layer of every comparator (a comparator joins the
 * current layer when both wires are still free there, else it opens a new
 * layer); pass 2 writes c[] in flat order and records the layer boundaries.
 * Doing it in two passes keeps the layer table and the comparator array
 * consistent by construction -- the one-pass version kept shifting the
 * start[] table without moving the data, which corrupted the network.
 *
 * Correctness: comparators inside one layer touch disjoint wires and
 * therefore commute, so the layered network computes the same function as
 * the original flat sequence. */
static void layer_from_flat(const int flat[][2], int cnt, Net *n)
{
    static int lay[MAXC];
    static unsigned char used[MAXL][NCH];
    memset(used, 0, sizeof(used));

    int cur = 0, L = 0;
    for (int i = 0; i < cnt; i++) {
        int a = flat[i][0], b = flat[i][1];
        if (L == 0) { L = 1; cur = 0; }
        else if (used[cur][a] || used[cur][b]) {
            if (cur + 1 >= MAXL) { lay[i] = cur; continue; }
            cur++; L = cur + 1;
        }
        used[cur][a] = 1; used[cur][b] = 1;
        lay[i] = cur;
    }

    n->n = 0; n->L = L;
    for (int i = 0; i <= MAXL; i++) n->start[i] = 0;
    int last = -1;
    for (int i = 0; i < cnt; i++) {
        if (lay[i] != last) { last = lay[i]; n->start[last] = n->n; }
        if (n->n >= MAXC) break;
        n->c[n->n][0] = flat[i][0];
        n->c[n->n][1] = flat[i][1];
        n->n++;
    }
    if (L > 0) n->start[L] = n->n;
}

/* ---- baseline loading -------------------------------------------------- */
static int load_baseline(const char *path, int flat[][2], int *cnt)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return -1; }
    int ch, v[2], c = 0;
    *cnt = 0;
    while ((ch = fgetc(f)) != EOF) {
        if (ch == '[') {
            if (fscanf(f, "%d,%d", &v[0], &v[1]) == 2) {
                if (c < MAXC) { flat[c][0] = v[0]; flat[c][1] = v[1]; c++; }
            }
        }
    }
    fclose(f);
    *cnt = c;
    return c;
}

/* ---- GA ---------------------------------------------------------------- */
#define POPMAX 256
static Net pop[POPMAX], best;
static long fit[POPMAX];
static int maxC;

/* integrity check: wire indices must be valid and the layer table must be
 * monotone with start[L] == n */
static int net_check(const Net *n)
{
    if (n->n < 0 || n->n > MAXC) return 0;
    for (int i = 0; i < n->n; i++) {
        int a = n->c[i][0], b = n->c[i][1];
        if (a < 0 || a >= NCH || b < 0 || b >= NCH) return 0;
    }
    if (n->L < 0 || n->L > MAXL) return 0;
    for (int t = 0; t < n->L; t++)
        if (n->start[t] > n->start[t + 1]) return 0;
    if (n->start[n->L] != n->n) return 0;
    return 1;
}

static long fitness(Net *n)
{
    if (!net_check(n)) {
        fprintf(stderr, "BAD NET: n=%d L=%d start[L]=%d\n", n->n, n->L, n->start[n->L]);
        for (int i = 0; i < n->n && i < 12; i++)
            fprintf(stderr, "   c[%d] = (%d,%d)\n", i, n->c[i][0], n->c[i][1]);
        exit(3);
    }
    long good = evaluate(n);
    if (good == TOTAL) return 1000000000000L + (MAXC - n->n);
    return good;
}

static void copy_net(Net *d, const Net *s)
{
    memcpy(d->c, s->c, sizeof(s->c));
    memcpy(d->start, s->start, sizeof(s->start));
    d->L = s->L; d->n = s->n;
}

static void mutate(Net *n)
{
    int kind = rnd(4);
    if (n->L == 0) return;
    if (kind == 0) {
        /* change the wires of a random comparator */
        int t = rnd(n->L);
        if (n->start[t + 1] <= n->start[t]) return;
        int i = n->start[t] + rnd(n->start[t + 1] - n->start[t]);
        int a = rnd(NCH), b = rnd(NCH);
        if (a == b) b = (b + 1) % NCH;
        if (a > b) { int tmp = a; a = b; b = tmp; }
        n->c[i][0] = a; n->c[i][1] = b;
    } else if (kind == 1) {
        /* move a comparator to another layer (and resolve conflicts) */
        int t = rnd(n->L);
        if (n->start[t + 1] <= n->start[t]) return;
        int i = n->start[t] + rnd(n->start[t + 1] - n->start[t]);
        int a = n->c[i][0], b = n->c[i][1];
        int lt = rnd(n->L);
        layer_remove(n, t, i - n->start[t]);
        layer_place(n, lt, a, b, maxC);
    } else if (kind == 2) {
        /* drop a comparator (we want fewer) */
        int t = rnd(n->L);
        if (n->start[t + 1] <= n->start[t]) return;
        int i = n->start[t] + rnd(n->start[t + 1] - n->start[t]);
        layer_remove(n, t, i - n->start[t]);
    } else {
        /* add a comparator */
        int a = rnd(NCH), b = rnd(NCH);
        if (a == b) b = (b + 1) % NCH;
        if (a > b) { int tmp = a; a = b; b = tmp; }
        int lt = rnd(n->L + 1);
        if (lt > n->L) { n->L++; n->start[n->L] = n->n; }
        layer_place(n, lt, a, b, maxC);
        if (n->L < MAXL) n->start[n->L + 1] = n->n;
    }
    /* repair: a layer must not have two comparators on the same wire */
    for (int t = 0; t < n->L; t++) {
        for (int i = n->start[t]; i < n->start[t + 1]; i++) {
            for (int j = i + 1; j < n->start[t + 1]; j++) {
                int a = n->c[i][0], b = n->c[i][1];
                int x = n->c[j][0], y = n->c[j][1];
                if (a == x || a == y || b == x || b == y) {
                    /* drop the later one */
                    layer_remove(n, t, j - n->start[t]);
                    j--;
                }
            }
        }
    }
    if (n->L < MAXL) n->start[n->L + 1] = n->n;
}

static void crossover(const Net *A, const Net *B, Net *C)
{
    int cut = 1 + rnd(A->L > 1 ? A->L - 1 : 1);
    C->n = 0; C->L = 0;
    for (int i = 0; i <= MAXL; i++) C->start[i] = 0;   /* was uninitialised:
                                               layer_place reads start[t+1] */
    for (int t = 0; t < cut && t < A->L; t++) {
        for (int i = A->start[t]; i < A->start[t + 1]; i++) {
            int a = A->c[i][0], b = A->c[i][1];
            layer_place(C, C->L, a, b, maxC);
        }
        C->L++;
        C->start[C->L] = C->n;
    }
    for (int t = cut; t < B->L; t++) {
        for (int i = B->start[t]; i < B->start[t + 1]; i++) {
            int a = B->c[i][0], b = B->c[i][1];
            layer_place(C, C->L, a, b, maxC);
        }
        C->L++;
        C->start[C->L] = C->n;
    }
    if (C->L > MAXL) C->L = MAXL;
    C->start[C->L] = C->n;
}

static void write_net(const char *path, const Net *n)
{
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }
    fprintf(f, "{\"comparators\": [");
    for (int t = 0; t < n->L; t++)
        for (int i = n->start[t]; i < n->start[t + 1]; i++)
            fprintf(f, "[%d,%d],", n->c[i][0], n->c[i][1]);
    fprintf(f, "], \"layers\": %d}\n", n->L);
    fclose(f);
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: ganet <baseline.json> <seconds> <seed> <out.json> [pop] [maxC]\n");
        return 1;
    }
    const char *base = argv[1];
    double budget = atof(argv[2]);
    uint64_t seed = strtoull(argv[3], NULL, 10);
    const char *outp = argv[4];
    int P = argc > 5 ? atoi(argv[5]) : 40;
    maxC = argc > 6 ? atoi(argv[6]) : 70;
    if (P < 4) P = 4;
    if (P > POPMAX) P = POPMAX;
    rs ^= seed * 0x9E3779B97F4A7C15ULL + 1;

    int flat[MAXC][2], cnt = 0;
    if (load_baseline(base, flat, &cnt) <= 0) return 1;
    Net seednet;
    layer_from_flat(flat, cnt, &seednet);
    fprintf(stderr, "baseline: %d comparators in %d layers\n", seednet.n, seednet.L);
    if (!net_check(&seednet)) {
        fprintf(stderr, "SEED NET BROKEN: n=%d L=%d start[L]=%d cnt=%d\n",
                seednet.n, seednet.L, seednet.start[seednet.L], cnt);
        for (int i = 0; i < seednet.n; i++)
            fprintf(stderr, "  c[%d]=(%d,%d)\n", i, seednet.c[i][0], seednet.c[i][1]);
    }
    long sf = evaluate(&seednet);
    fprintf(stderr, "baseline sorts %ld/%ld inputs\n", sf, TOTAL);

    for (int i = 0; i < P; i++) {
        copy_net(&pop[i], &seednet);
        for (int m = 0; m < 3 + rnd(4); m++) mutate(&pop[i]);
        fit[i] = fitness(&pop[i]);
    }
    copy_net(&best, &pop[0]);
    long bestfit = fit[0];
    for (int i = 1; i < P; i++) if (fit[i] > bestfit) { bestfit = fit[i]; copy_net(&best, &pop[i]); }

    double t0 = wall();
    long gen = 0, evals = 0;
    while (wall() - t0 < budget) {
        gen++;
        /* elitism: keep the best */
        int worst = 0;
        for (int i = 1; i < P; i++) if (fit[i] < fit[worst]) worst = i;
        if (fit[worst] < bestfit) { copy_net(&pop[worst], &best); fit[worst] = bestfit; }
        /* offspring */
        static Net child[POPMAX];
        for (int i = 0; i < P; i++) {
            int a = rnd(P), b = rnd(P);
            while (b == a) b = rnd(P);
            /* tournament-ish: prefer fitter parents */
            if (fit[a] < fit[b]) { int t = a; a = b; b = t; }
            crossover(&pop[a], &pop[b], &child[i]);
            int nm = 1 + rnd(3);
            for (int m = 0; m < nm; m++) mutate(&child[i]);
            long f = fitness(&child[i]);
            evals++;
            if (f > fit[i]) { copy_net(&pop[i], &child[i]); fit[i] = f; }
            if (f > bestfit) {
                bestfit = f;
                copy_net(&best, &child[i]);
                write_net(outp, &best);
                fprintf(stderr, "gen %ld: new best fit=%ld n=%d layers=%d\n",
                        gen, f, best.n, best.L);
                if (best.n <= maxC && evaluate(&best) == TOTAL) {
                    fprintf(stderr, "*** SORTING NETWORK WITH %d COMPARATORS FOUND ***\n", best.n);
                    write_net(outp, &best);
                    break;
                }
            }
        }
    }
    write_net(outp, &best);
    long g = evaluate(&best);
    fprintf(stderr, "gen=%ld evals=%ld best: n=%d layers=%d sorts=%ld/%ld  (%.0f s)\n",
            gen, evals, best.n, best.L, g, TOTAL, wall() - t0);
    return 0;
}
