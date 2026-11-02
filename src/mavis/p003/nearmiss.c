/* nearmiss.c -- find 36-vertex (4,6)-Ramsey near-misses from the 37 known
 * valid 35-vertex graphs.
 *
 * Idea
 * ----
 * Let B be a valid (4,6)-graph on m vertices (m = 35, or 34 when we delete one
 * vertex u from a known 35-vertex graph).  Add a new vertex v with
 * neighbourhood S.  In the resulting (m+1)-vertex graph:
 *
 *     #K4 = #triangles of B fully inside S
 *     #I6 = #independent 5-sets of B fully inside V(B)\S
 *
 * so the total violation count is exactly
 *
 *     f(S) = tri(B[S]) + ind5(B[V\S]).
 *
 * The extension SAT is UNSAT for all 37 known graphs (so f >= 1), but f is a
 * tiny objective over m bits that simulated annealing minimises in
 * microseconds.  We minimise f over every (known graph, deleted vertex) pair,
 * i.e. 37 * 36 = 1332 base graphs, and report the best.
 *
 * Input : text file, one graph per line: "n m0 m1 ... m_{n-1}", where mi is a
 *         hex bitmask of the adjacency row (bit j set <=> edge i-j).
 * Output: best finding on stdout; optional JSON file with every base graph's
 *         best (f, S).
 *
 * Build: gcc -O2 -o nearmiss nearmiss.c -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MAXN 40
#define MAXI5 8192

typedef uint64_t mask;

static int   gn;
static mask  gadj[MAXN];
static mask  gtri[MAXN][512];
static int   gtri_n[MAXN];
static mask  gind5[MAXN][8192];
static int   gind5_n[MAXN];
static int   gtri_total, gind5_total;
static mask  gnonadj[MAXN];

static int popcount(mask m)
{
    int c = 0;
    while (m) { mask lo = m & (~m + 1); m ^= lo; c++; }
    return c;
}

static void rec_ind5(mask cand, int depth, int *chosen)
{
    if (depth == 5) {
        for (int a = 0; a < 5; a++) {
            mask om = 0;
            for (int b = 0; b < 5; b++)
                if (b != a) om |= ((mask)1 << chosen[b]);
            if (gind5_n[chosen[a]] >= 8192) {
                fprintf(stderr, "FATAL: ind5 buffer overflow\n"); exit(2);
            }
            gind5[chosen[a]][gind5_n[chosen[a]]++] = om;
        }
        gind5_total++;
        return;
    }
    while (cand) {
        mask lo = cand & (~cand + 1);
        int v = __builtin_ctzll(lo);
        cand ^= lo;
        chosen[depth] = v;
        rec_ind5(cand & gnonadj[v], depth + 1, chosen);
    }
}

static void build_index(void)
{
    int n = gn, i, j, k;
    mask full = ((mask)1 << n) - 1;

    memset(gtri, 0, sizeof(gtri));
    memset(gind5, 0, sizeof(gind5));
    memset(gtri_n, 0, sizeof(gtri_n));
    memset(gind5_n, 0, sizeof(gind5_n));
    gtri_total = gind5_total = 0;

    for (j = 0; j < n; j++) {
        mask gt = ~((((mask)1 << (j + 1)) - 1)) & full;
        for (i = 0; i < j; i++) {
            if (!((gadj[i] >> j) & 1)) continue;
            mask c = gadj[i] & gadj[j] & gt;
            while (c) {
                mask lo = c & (~c + 1);
                k = __builtin_ctzll(lo);
                c ^= lo;
                if (gtri_n[i] >= 512 || gtri_n[j] >= 512 || gtri_n[k] >= 512) {
                    fprintf(stderr, "FATAL: triangle buffer overflow\n"); exit(2);
                }
                gtri[i][gtri_n[i]++] = ((mask)1 << j) | ((mask)1 << k);
                gtri[j][gtri_n[j]++] = ((mask)1 << i) | ((mask)1 << k);
                gtri[k][gtri_n[k]++] = ((mask)1 << i) | ((mask)1 << j);
                gtri_total++;
            }
        }
    }

    for (i = 0; i < n; i++)
        gnonadj[i] = full & ~gadj[i] & ~((mask)1 << i);

    int chosen[8];
    rec_ind5(full, 0, chosen);

    /* self-check: every triangle must appear exactly 3x (once per vertex) and
     * every independent 5-set exactly 5x. */
    long tc = 0; for (i = 0; i < n; i++) tc += gtri_n[i];
    long ic = 0; for (i = 0; i < n; i++) ic += gind5_n[i];
    if (tc != 3L * gtri_total || ic != 5L * gind5_total) {
        fprintf(stderr, "FATAL: index self-check failed tri %ld/%d ind5 %ld/%d\n",
                tc, gtri_total, ic, gind5_total);
        exit(2);
    }
}

/* exact f(S): triangles fully in S + independent 5-sets fully in V\S */
static int f_exact(mask S)
{
    int n = gn, i, t, tri = 0, i5 = 0;
    mask full = ((mask)1 << n) - 1, comp = full & ~S;

    for (i = 0; i < n; i++) {
        if (!((S >> i) & 1)) continue;     /* i itself must be inside S */
        for (t = 0; t < gtri_n[i]; t++) {
            mask pm = gtri[i][t];
            if ((pm & S) == pm) {
                int b = __builtin_ctzll(pm);
                int c = __builtin_ctzll(pm & (pm - 1));
                if (i < b && i < c) tri++;
            }
        }
    }
    for (i = 0; i < n; i++) {
        if (!((comp >> i) & 1)) continue;   /* i itself must be outside S */
        for (t = 0; t < gind5_n[i]; t++) {
            mask rm = gind5[i][t];
            if ((rm & comp) == rm) {
                int ok = 1;
                mask m = rm;
                while (m) {
                    mask lo = m & (~m + 1);
                    int x = __builtin_ctzll(lo);
                    m ^= lo;
                    if (x < i) { ok = 0; break; }
                }
                if (ok) i5++;
            }
        }
    }
    return tri + i5;
}

static unsigned long long rng_state;

static double rnd(void)
{
    rng_state = rng_state * 6364136223846793005ULL + 1442695040888963407ULL;
    return (double)((rng_state >> 33) % 1000000ULL) / 1000000.0;
}

static int anneal(unsigned long long seed, long iters, int restarts, mask *bestS)
{
    int n = gn, r;
    int bestf = 1 << 30;
    mask best = 0;

    rng_state = seed * 6364136223846793005ULL + 1442695040888963407ULL;

    for (r = 0; r < restarts; r++) {
        mask S = 0;
        for (int i = 0; i < n; i++) if (rnd() < 0.5) S |= ((mask)1 << i);
        int cur = f_exact(S);
        for (long it = 0; it < iters; it++) {
            double T = 3.0 * pow(0.02 / 3.0, (double)it / (double)iters);
            int v = (int)(rnd() * n);
            if (v >= n) v = n - 1;
            int inS = (S >> v) & 1;
            int d = 0, t;
            for (t = 0; t < gtri_n[v]; t++) {
                mask pm = gtri[v][t];
                if ((pm & S) == pm) d += inS ? -1 : 1;
            }
            for (t = 0; t < gind5_n[v]; t++) {
                mask rm = gind5[v][t];
                if ((rm & S) == 0) d += inS ? 1 : -1;
            }
            if (d <= 0 || rnd() < exp(-d / T)) {
                S ^= ((mask)1 << v);
                cur += d;
            }
            if (cur < bestf) {
                bestf = cur;
                best = S;
                if (bestf == 0) { *bestS = best; return bestf; }
            }
        }
    }
    *bestS = best;
    return bestf;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: nearmiss <graphs.txt> [out.json] [iters] [restarts]\n"); return 1; }
    long ITERS = (argc > 3) ? atol(argv[3]) : 20000;
    int  RESTARTS = (argc > 4) ? atoi(argv[4]) : 4;
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror(argv[1]); return 1; }

    long gidx = 0;
    int global_best = 1 << 30, global_g = -1, global_u = -1;
    mask global_S = 0;
    FILE *jf = (argc > 2) ? fopen(argv[2], "w") : NULL;
    if (jf) fprintf(jf, "[\n");

    int n;
    while (fscanf(fp, "%d", &n) == 1) {
        mask rows[MAXN];
        for (int i = 0; i < n; i++) {
            unsigned long long h;
            if (fscanf(fp, "%llx", &h) != 1) { fprintf(stderr, "short read\n"); return 1; }
            rows[i] = (mask)h;
        }

        for (int u = -1; u < n; u++) {
            int map[MAXN], m = 0;
            for (int i = 0; i < n; i++) if (i != u) map[m++] = i;
            gn = m;
            for (int i = 0; i < m; i++) {
                mask r = 0;
                for (int j = 0; j < m; j++)
                    if (rows[map[i]] >> map[j] & 1) r |= ((mask)1 << j);
                gadj[i] = r;
            }
            build_index();

            mask bestS = 0;
            int f = anneal(1234u + 7919u * (unsigned)(gidx * 40 + (u + 1)),
                           ITERS, RESTARTS, &bestS);
            int f2 = f_exact(bestS);
            if (f2 != f) fprintf(stderr, "MISMATCH g%ld u=%d f=%d f2=%d\n", gidx, u, f, f2);

            if (f < global_best) {
                global_best = f; global_g = (int)gidx; global_u = u; global_S = bestS;
                fprintf(stderr, "g%ld u=%d  f=%d (base=%d tri=%d ind5=%d)\n",
                        gidx, u, f, gn, gtri_total, gind5_total);
                fflush(stderr);
            }
            if (jf)
                fprintf(jf, "  {\"g\": %ld, \"u\": %d, \"f\": %d, \"S\": %llu},\n",
                        gidx, u, f, (unsigned long long)bestS);
        }
        gidx++;
    }
    fclose(fp);
    if (jf)
        fprintf(jf, "  {\"best_g\": %d, \"best_u\": %d, \"best_f\": %d, \"best_S\": %llu}\n]\n",
                global_g, global_u, global_best, (unsigned long long)global_S);
    printf("BEST g=%d u=%d f=%d S=%llu\n", global_g, global_u, global_best,
           (unsigned long long)global_S);
    return 0;
}
