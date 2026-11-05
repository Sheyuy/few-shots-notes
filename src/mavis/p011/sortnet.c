/* sortnet.c -- stochastic search for a short sorting network on 17 channels.
 *
 * Problem p011: the board baseline is a 71-comparator network on 17 channels;
 * we need one with <= 70 comparators.  Nobody has published 70 for n = 17
 * (the classical best known was 75, so the board baseline is already better
 * than the literature).
 *
 * Method: the 0-1 principle says a comparator network sorts all inputs iff it
 * sorts all 2^17 binary inputs.  So the fitness of a candidate is "how many of
 * the 131072 input patterns come out sorted".
 *
 * Speed: the patterns are simulated 64 at a time with bit-slicing (one
 * uint64 per wire holds the output bit of 64 different inputs), so a
 * compare-swap is just two ANDs and two ORs, and the sorted-ness of a block is
 * one OR of the 16 adjacent-violation masks.  A full evaluation costs about
 * 1 ms instead of ~30 ms.
 *
 * Search: start from the baseline, then repeatedly delete one comparator and
 * try to repair the resulting network by random mutations (replace / swap /
 * insert a comparator), accepting a candidate when it sorts more inputs than
 * its parent.  Any candidate that sorts *everything* with fewer comparators
 * is recorded.
 *
 * Usage: ./sortnet <baseline.json> <seconds> <seed> <out.json>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define NCH 17
#define MAXC 256
#define TOTAL (1L << NCH)

typedef struct {
    int c[MAXC][2];
    int n;
} Net;

static uint64_t rs = 0x243F6A8885A308D3ULL;
static inline uint64_t rnd64(void) { rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs; }
static inline int rnd(int k) { return (int)(rnd64() % (uint64_t)k); }

static double wall(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

/* ---- bit-sliced evaluation ------------------------------------------- */
static uint64_t wire[NCH];

static long evaluate(const Net *net)
{
    long good = 0;
    uint64_t pats[64];
    for (long base = 0; base < TOTAL; base += 64) {
        for (int k = 0; k < 64; k++) {
            uint64_t p = (uint64_t)(base + k);
            pats[k] = p;
        }
        for (int i = 0; i < NCH; i++) {
            uint64_t w = 0;
            if (i < 6) {
                for (int k = 0; k < 64; k++)
                    if ((pats[k] >> i) & 1ULL) w |= 1ULL << k;
            } else {
                /* bits >= 6 do not change inside a 64-block */
                if ((base >> i) & 1ULL) w = ~0ULL; else w = 0ULL;
            }
            wire[i] = w;
        }
        for (int t = 0; t < net->n; t++) {
            int a = net->c[t][0], b = net->c[t][1];
            uint64_t x = wire[a], y = wire[b];
            wire[a] = x & y;      /* min */
            wire[b] = x | y;      /* max */
        }
        uint64_t unsorted = 0;
        for (int i = 0; i + 1 < NCH; i++)
            unsorted |= wire[i] & ~wire[i + 1];
        good += 64 - __builtin_popcountll(unsorted);
    }
    return good;
}

/* ---- mutations -------------------------------------------------------- */
static void mut_replace(Net *n)
{
    int t = rnd(n->n);
    int a = rnd(NCH), b = rnd(NCH);
    if (a == b) b = (b + 1) % NCH;
    if (a > b) { int t2 = a; a = b; b = t2; }
    n->c[t][0] = a; n->c[t][1] = b;
}
static void mut_swap(Net *n)
{
    if (n->n < 2) return;
    int i = rnd(n->n), j = rnd(n->n);
    while (j == i) j = rnd(n->n);
    int a = n->c[i][0], b = n->c[i][1];
    n->c[i][0] = n->c[j][0]; n->c[i][1] = n->c[j][1];
    n->c[j][0] = a; n->c[j][1] = b;
}
static void mut_insert(Net *n)
{
    if (n->n >= MAXC) return;
    int a = rnd(NCH), b = rnd(NCH);
    if (a == b) b = (b + 1) % NCH;
    if (a > b) { int t = a; a = b; b = t; }
    int pos = rnd(n->n + 1);
    for (int i = n->n; i > pos; i--) { n->c[i][0] = n->c[i-1][0]; n->c[i][1] = n->c[i-1][1]; }
    n->c[pos][0] = a; n->c[pos][1] = b;
    n->n++;
}
static void mut_delete(Net *n)
{
    if (n->n <= 1) return;
    int t = rnd(n->n);
    for (int i = t; i + 1 < n->n; i++) { n->c[i][0] = n->c[i+1][0]; n->c[i][1] = n->c[i+1][1]; }
    n->n--;
}

/* ---- baseline loading ------------------------------------------------- */
static int load_baseline(const char *path, Net *n)
{
    FILE *f = fopen(path, "r");
    if (!f) { perror(path); return -1; }
    int ch, cnt = 0, v[2];
    n->n = 0;
    /* crude scan for "a","b" pairs */
    while ((ch = fgetc(f)) != EOF) {
        if (ch == '[') {
            if (fscanf(f, "%d,%d", &v[0], &v[1]) == 2) {
                if (cnt < MAXC) { n->c[cnt][0] = v[0]; n->c[cnt][1] = v[1]; cnt++; }
            }
        }
    }
    fclose(f);
    n->n = cnt;
    return cnt;
}

static void write_out(const char *path, const Net *n)
{
    FILE *f = fopen(path, "w");
    if (!f) { perror(path); return; }
    fprintf(f, "{\"comparators\":[");
    for (int i = 0; i < n->n; i++)
        fprintf(f, "%s[%d,%d]", i ? "," : "", n->c[i][0], n->c[i][1]);
    fprintf(f, "]}\n");
    fclose(f);
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s <baseline.json> <seconds> <seed> <out.json>\n", argv[0]);
        return 1;
    }
    const char *basefile = argv[1];
    double limit = atof(argv[2]);
    rs ^= (uint64_t)atoll(argv[3]) * 0x9E3779B97F4A7C15ULL;

    Net base, cur, cand, best;
    memset(&base, 0, sizeof base);
    if (load_baseline(basefile, &base) <= 0) { fprintf(stderr, "bad baseline\n"); return 1; }
    cur = base; best = base;
    long bestfit = evaluate(&best);
    fprintf(stderr, "baseline: %d comparators, sorts %ld/%ld inputs\n", best.n, bestfit, TOTAL);
    if (bestfit < TOTAL) fprintf(stderr, "WARNING: baseline does not sort everything\n");

    double t0 = wall();
    long evals = 0, accepted = 0, found = 0, attempts = 0;
    const long STALL_LIMIT = 400;   /* give up on a repair after this many fruitless steps */
    const long ATTEMPT_LIMIT = 3000; /* cap one delete+repair attempt so many deletions get tried */
    while (wall() - t0 < limit) {
        /* one attempt: delete one comparator from the best network, then repair
         * it by hill climbing on the number of sorted input patterns */
        Net cur = best;
        mut_delete(&cur);
        long curfit = evaluate(&cur);
        evals++;
        attempts++;
        long stall = 0;
        long steps = 0;
        while (wall() - t0 < limit && stall < STALL_LIMIT && steps < ATTEMPT_LIMIT) {
            steps++;
            Net cand = cur;
            int r = rnd(3);
            if (r == 0) mut_replace(&cand);
            else if (r == 1) mut_swap(&cand);
            else mut_insert(&cand);      /* a repair may need a comparator elsewhere */
            long f = evaluate(&cand);
            evals++;
            if (f >= curfit) {          /* accept improvements and plateaus */
                cur = cand;
                if (f > curfit) accepted++;
                curfit = f;
                stall = 0;
            } else {
                stall++;
            }
            /* only a STRICTLY SHORTER network counts as an improvement: with the
             * insert mutation the repair can produce a longer (but still sorting)
             * network, and taking that as "best" made the counter explode while
             * the network actually got worse (71 -> 254 comparators). */
            if (curfit == TOTAL && cur.n < best.n) {
                best = cur;
                found++;
                fprintf(stderr, "\n*** %d comparators that sort everything (eval #%ld)\n",
                        best.n, evals);
                write_out(argv[4], &best);
                break;
            }
        }
    }
    fprintf(stderr, "evals=%ld accepted=%ld attempts=%ld improvements=%ld time=%.0fs\n",
            evals, accepted, attempts, found, wall() - t0);
    fprintf(stderr, "best: %d comparators, sorts %ld/%ld inputs\n", best.n, bestfit, TOTAL);
    write_out(argv[4], &best);
    if (bestfit == TOTAL && best.n < base.n) {
        fprintf(stderr, "IMPROVEMENT FOUND\n");
        return 0;
    }
    return 1;
}
