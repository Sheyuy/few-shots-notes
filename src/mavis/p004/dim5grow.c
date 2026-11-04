/* dim5grow.c -- grow maximal independent sets in C7^5 (16807 vertices) from
 * random seeds and report their size distribution.
 *
 * Why: the Polak-Schrijver 367-point set is a special algebraic object.  If
 * random restarts can only reach ~330, then the 367-point set is probably
 * unique up to isomorphism, which would close the whole "find another base
 * code" branch of p004 (and explain why "delete k, add k+1" found nothing).
 *
 * The graph: vertices are Z_7^5; x ~ y iff |x_i - y_i|_Lee <= 1 in EVERY
 * coordinate (the square strong product), so a point's closed neighbourhood
 * has 3^5 = 243 points.
 *
 * Usage: ./dim5grow <restarts> <seed> [outfile]
 *        each restart: start from a random vertex, repeatedly add a uniformly
 *        random compatible vertex until none is left (a maximal independent
 *        set), record its size; write the largest few sets to <outfile>
 *        (one per line, as 5-digit base-7 strings).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DIM 5
#define CYC 7
#define NV 16807
#define NBR 242

static uint8_t dig[NV][DIM];
static uint16_t nbr[NV][NBR + 1];
static uint8_t inS[NV];
static uint8_t blocked[NV];     /* adjacent to something in S */

static uint64_t rs = 88172645463325252ULL;
static inline uint64_t rnd64(void)
{
    rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs;
}
static inline uint32_t rnd(uint32_t n) { return (uint32_t)(rnd64() % n); }

static void gen(void)
{
    for (int v = 0; v < NV; v++) {
        int t = v;
        for (int k = DIM - 1; k >= 0; k--) { dig[v][k] = (uint8_t)(t % CYC); t /= CYC; }
    }
    for (int v = 0; v < NV; v++) {
        int m = 0;
        for (int e = 0; e <= NBR; e++) {
            int x = e, idx = 0, val[DIM];
            for (int k = 0; k < DIM; k++) { int f = x % 3; x /= 3; val[k] = (dig[v][k] - 1 + f + CYC) % CYC; }
            for (int k = 0; k < DIM; k++) idx = idx * CYC + val[k];
            nbr[v][m++] = (uint16_t)idx;
        }
    }
}

static void add_vertex(int v)
{
    inS[v] = 1;
    for (int j = 0; j <= NBR; j++) blocked[nbr[v][j]] = 1;
}

/* random maximal independent set */
static int grow(void)
{
    memset(inS, 0, NV);
    memset(blocked, 0, NV);
    int start = (int)rnd(NV);
    add_vertex(start);
    int cnt = 1;
    /* repeatedly add a random compatible vertex; scan from a random offset so
     * the choice is uniform over the available ones */
    for (;;) {
        int pick = -1;
        int off = (int)rnd(NV);
        for (int t = 0; t < NV; t++) {
            int v = (off + t) % NV;
            if (!inS[v] && !blocked[v]) { pick = v; break; }
        }
        if (pick < 0) break;
        add_vertex(pick);
        cnt++;
    }
    return cnt;
}

static void dump(FILE *f)
{
    for (int v = 0; v < NV; v++)
        if (inS[v]) fprintf(f, "%d%d%d%d%d ", dig[v][0], dig[v][1], dig[v][2], dig[v][3], dig[v][4]);
    fprintf(f, "\n");
}

int main(int argc, char **argv)
{
    long restarts = (argc > 1) ? atol(argv[1]) : 100;
    rs ^= (uint64_t)(argc > 2 ? atol(argv[2]) : 1) * 0x9E3779B97F4A7C15ULL;
    const char *out = (argc > 3) ? argv[3] : NULL;
    gen();

    long hist[NV + 1];
    memset(hist, 0, sizeof(hist));
    int best = 0;
    char *bestset = NULL;
    for (long r = 0; r < restarts; r++) {
        int sz = grow();
        hist[sz]++;
        if (sz > best) {
            best = sz;
            free(bestset);
            bestset = malloc((size_t)NV * 8);
            FILE *tmp = tmpfile();
            if (tmp) { dump(tmp); rewind(tmp); size_t n = fread(bestset, 1, (size_t)NV * 8, tmp); (void)n; fclose(tmp); }
        }
        if ((r + 1) % 10 == 0) fprintf(stderr, "\rrestart %ld/%ld best=%d", r + 1, restarts, best);
    }
    fprintf(stderr, "\n");
    printf("restarts=%ld best=%d\n", restarts, best);
    printf("size histogram (top):\n");
    /* print a compact histogram of the sizes seen */
    int lo = 1, hi = 0;
    for (int i = 0; i <= NV; i++) if (hist[i]) { if (!lo) lo = i; hi = i; }
    for (int i = lo; i <= hi; i++)
        if (hist[i]) printf("  size %d : %ld times\n", i, hist[i]);
    if (out && bestset) {
        FILE *f = fopen(out, "w");
        if (f) { dump(f); fclose(f); printf("best set written to %s\n", out); }
    }
    return 0;
}
