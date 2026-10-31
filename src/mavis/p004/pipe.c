/* pipe.c -- replicate and search the Polak-Schrijver construction for
 * independent sets in C7^5 (the "gadget" behind the p004 record).
 *
 * Pipeline (arXiv:1808.07438, Section 3):
 *   (i)   S = { t * (1, q, q^2, q^3, q^4)  mod n : t in Z_n }   (a cyclic code)
 *   (ii)  add a constant word o (mod n)
 *   (iii) replace every letter i in [0,n) by floor(i / f),  f = n/7
 *   (iv)  keep an independent subset M of the n mapped words
 *   (v)   extend M by a maximum independent set of the compatible subgraph G
 *   score = |M| + alpha(G)
 *
 * Published instance: n=382, q=7, o=(40,123,40,123,40), f=54.5 gives
 * |M|=327, alpha(G)=40, total 367 -- reproduced exactly by this program.
 *
 * Two choices for step (iv) are tried: the paper's "drop every word that
 * touches another" rule, and the exact maximum independent set of the
 * conflict graph on the same n words.
 *
 * Modes:
 *   one    n q o0..o4 [f]                        evaluate one parameter tuple
 *   list   nmin nmax qmin qmax lo hi             list (n,q) with n/k in [lo,hi]
 *   sweep  nmin nmax qmin qmax seed noff pat     search (n,q) x offsets
 *   sweepv nmin nmax iters seed                  search random multiplier vectors
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
static double now_s(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec+1e-9*ts.tv_nsec;}

#define DIM 5
#define CYC 7
#define NV  16807
#define W   (NV / 64 + 1)      /* 263 */
#define MAXG 4096
#define GW   (MAXG / 64 + 1)   /* 65   */

static uint8_t  dig[NV][DIM];
static uint16_t nbr[NV][243];  /* closed neighbourhood in C7^5, 3^5 = 243 */

static int g_dbg_verbose = 0;
static uint64_t rs = 88172645463325252ULL;
static inline uint64_t rnd(void) {
    rs ^= rs << 13; rs ^= rs >> 7; rs ^= rs << 17; return rs;
}

static void gen_graph(void) {
    for (int v = 0; v < NV; v++) {
        int t = v;
        for (int k = DIM - 1; k >= 0; k--) { dig[v][k] = (uint8_t)(t % CYC); t /= CYC; }
    }
    for (int v = 0; v < NV; v++) {
        int m = 0;
        for (int e = 0; e < 243; e++) {
            int x = e, idx = 0, val[DIM];
            for (int k = 0; k < DIM; k++) { int fq = x % 3; x /= 3; val[k] = (dig[v][k] - 1 + fq + CYC) % CYC; }
            for (int k = 0; k < DIM; k++) idx = idx * CYC + val[k];
            nbr[v][m++] = (uint16_t)idx;
        }
    }
}

/* ------------------------------------------------------ exact MIS on G ---- */
/* G is a sparse induced subgraph of C7^5.
 *
 * Upper bound for alpha: a proper colouring of the COMPLEMENT of G partitions
 * the vertices into cliques of G; an independent set meets every clique in at
 * most one vertex, so the number of complement colours bounds alpha from
 * above.  (Colouring G itself does NOT bound alpha: a colour class is an
 * independent set, so it may hold many vertices of one independent set.)
 *
 * Reduction: a vertex of degree 0 can always be taken; a vertex of degree 1
 * can always be taken as well (if its neighbour is in the solution, swap).
 */

static uint64_t gm[GW];
static uint64_t gadj[MAXG][GW];
static int      gn;
static int      col_[MAXG], stamp_[MAXG], call_id_;
static int      g_found;
static long     g_nodes, g_node_budget;
static int      g_exact;
static uint64_t g_sel[GW];
static uint64_t g_best[GW];

static void mis_clear(void) { memset(gm, 0, sizeof(gm)); gn = 0; }

static int color_bound(const uint64_t *mask) {
    call_id_++;
    int ncolors = 0;
    for (int i = 0; i < GW; i++) {
        uint64_t w = mask[i];
        while (w) {
            int b = __builtin_ctzll(w); w &= w - 1;
            int u = i * 64 + b;
            uint64_t used = 0;
            for (int j = 0; j < GW; j++) {
                uint64_t z = mask[j];
                while (z) {
                    int c = __builtin_ctzll(z); z &= z - 1;
                    int y = j * 64 + c;
                    if (stamp_[y] != call_id_) continue;
                    if (gadj[u][y / 64] >> (y % 64) & 1ULL) continue;
                    used |= 1ULL << col_[y];
                }
            }
            int c = 0;
            while (used & (1ULL << c)) c++;
            col_[u] = c;
            stamp_[u] = call_id_;
            if (c + 1 > ncolors) ncolors = c + 1;
        }
    }
    return ncolors;
}

/* Vertices of degree 0 can always be taken; a vertex u of degree 1 can always
 * be taken as well (if its unique neighbour v is in the solution, swap u for
 * v), but then v is EXCLUDED.  `inc` receives the vertices taken. */
static int reduce_mask(uint64_t *mask, uint64_t *inc) {
    int forced = 0;
    for (;;) {
        int did = 0;
        for (int i = 0; i < GW && !did; i++) {
            uint64_t w = mask[i];
            while (w) {
                int b = __builtin_ctzll(w); w &= w - 1;
                int u = i * 64 + b;
                int deg = 0, nb = -1;
                for (int j = 0; j < GW; j++) {
                    uint64_t z = mask[j] & gadj[u][j];
                    while (z) {
                        int c = __builtin_ctzll(z); z &= z - 1;
                        int y = j * 64 + c;
                        if (y == u) continue;
                        deg++; nb = y;
                    }
                }
                if (deg == 0) {
                    mask[u / 64] &= ~(1ULL << (u % 64));
                    inc[u / 64] |= 1ULL << (u % 64);
                    forced++; did = 1; break;
                }
                if (deg == 1) {
                    mask[u / 64] &= ~(1ULL << (u % 64));
                    inc[u / 64] |= 1ULL << (u % 64);
                    mask[nb / 64] &= ~(1ULL << (nb % 64));   /* v is excluded */
                    forced++; did = 1; break;
                }
            }
        }
        if (!did) break;
    }
    return forced;
}

static void mis_find(uint64_t *mask, int cur, int target) {
    if (g_found) return;
    if (++g_nodes > g_node_budget) { g_exact = 0; return; }
    int any = 0;
    for (int i = 0; i < GW; i++) if (mask[i]) { any = 1; break; }
    if (!any) { if (cur >= target) g_found = 1; return; }
    {
        uint64_t inc[GW];
        memset(inc, 0, sizeof(inc));
        int f = reduce_mask(mask, inc);
        for (int i = 0; i < GW; i++) g_sel[i] |= inc[i];
        cur += f;
        any = 0;
        for (int i = 0; i < GW; i++) if (mask[i]) { any = 1; break; }
        if (!any) { if (cur >= target) g_found = 1; return; }
    }
    if (cur + color_bound(mask) < target) return;

    int bestv = -1, bestdeg = -1;
    for (int i = 0; i < GW; i++) {
        uint64_t w = mask[i];
        while (w) {
            int b = __builtin_ctzll(w); w &= w - 1;
            int u = i * 64 + b;
            int deg = 0;
            for (int j = 0; j < GW; j++) deg += __builtin_popcountll(mask[j] & gadj[u][j]);
            if (deg > bestdeg) { bestdeg = deg; bestv = u; }
        }
    }
    {   /* include bestv */
        uint64_t nm[GW], saved[GW];
        memcpy(nm, mask, sizeof(nm));
        memcpy(saved, g_sel, sizeof(saved));
        for (int j = 0; j < GW; j++) {
            uint64_t z = nm[j] & gadj[bestv][j];
            while (z) { int c = __builtin_ctzll(z); z &= z - 1; nm[j] &= ~(1ULL << c); }
        }
        nm[bestv / 64] &= ~(1ULL << (bestv % 64));
        g_sel[bestv / 64] |= 1ULL << (bestv % 64);
        mis_find(nm, cur + 1, target);
        memcpy(g_sel, saved, sizeof(saved));   /* undo forced/selected bits */
    }
    if (g_found) return;
    {   /* exclude bestv */
        uint64_t nm[GW];
        memcpy(nm, mask, sizeof(nm));
        nm[bestv / 64] &= ~(1ULL << (bestv % 64));
        mis_find(nm, cur, target);
    }
}

/* exact alpha of the graph currently in gm/gadj; out receives the choice */
static int alpha_exact(uint64_t *out) {
    int L = 0;
    memset(g_best, 0, sizeof(g_best));
    g_nodes = 0;                      /* budget is global over all iterations */
    for (;;) {
        g_found = 0; g_exact = 1;
        uint64_t nm[GW];
        memcpy(nm, gm, sizeof(nm));
        memset(g_sel, 0, sizeof(g_sel));
        mis_find(nm, 0, L + 1);
        if (!g_found) break;
        if (!g_exact) return -1;
        L++;
        memcpy(g_best, g_sel, sizeof(g_best));
    }
    if (out) memcpy(out, g_best, sizeof(g_best));
    return L;
}

/* --------------------------------------------------------- the pipeline -- */

static uint8_t  occ[NV];
static uint16_t cntN[NV];
static uint8_t  inlist[NV];
static int      idxmap[NV];

/* exact maximum independent set of the conflict graph on the mapped words */
static int conflict_mis(int *Wlist, int nw, int *out) {
    memset(inlist, 0, sizeof(inlist));
    for (int i = 0; i < nw; i++) inlist[Wlist[i]] = 1;
    int g = 0;
    for (int i = 0; i < nw; i++) {
        int w = Wlist[i];
        if (!inlist[w]) continue;
        if (idxmap[w] == -1 || 1) { }
    }
    for (int i = 0; i < nw; i++) idxmap[Wlist[i]] = -1;
    g = 0;
    for (int i = 0; i < nw; i++) {
        int w = Wlist[i];
        if (idxmap[w] == -1) idxmap[w] = g++;
    }
    if (g > MAXG) return -1;
    mis_clear();
    memset(gadj, 0, sizeof(uint64_t) * GW * g);
    for (int i = 0; i < nw; i++) {
        int w = Wlist[i];
        int iw = idxmap[w];
        gm[iw / 64] |= 1ULL << (iw % 64);
        for (int j = 0; j < 243; j++) {
            int x = nbr[w][j];
            if (!inlist[x]) continue;
            int ix = idxmap[x];
            if (ix == iw) continue;
            gadj[iw][ix / 64] |= 1ULL << (ix % 64);
        }
    }
    gn = g;
    uint64_t best[GW];
    double t0 = now_s();
    int a;
    if (g > 400) {
        /* candidate graph too large for an exact MIS: greedy maximal set */
        memset(best, 0, sizeof(uint64_t) * GW);
        uint64_t used[GW];
        memset(used, 0, sizeof(uint64_t) * GW);
        int cnt = 0;
        for (int iu = 0; iu < g; iu++) {
            if (used[iu / 64] >> (iu % 64) & 1ULL) continue;
            int ok = 1;
            for (int j = 0; j < 263 && ok; j++)
                if (gadj[iu][j] && (used[j / 64] >> (j % 64) & 1ULL)) ok = 0;
            if (!ok) continue;
            used[iu / 64] |= 1ULL << (iu % 64);
            best[iu / 64] |= 1ULL << (iu % 64);
            cnt++;
        }
        a = cnt;
    } else
        a = alpha_exact(best);
    double t1 = now_s();
    if (g_dbg_verbose) fprintf(stderr, "   extend: |G|=%d alpha=%d nodes=%ld time=%.3fs\n", g, a, g_nodes, t1 - t0);
    if (a < 0) return -1;
    int c = 0;
    for (int i = 0; i < nw; i++) {
        int w = Wlist[i];
        int iw = idxmap[w];
        if (best[iw / 64] >> (iw % 64) & 1ULL) out[c++] = w;
    }
    return c;
}

/* extend M by a maximum independent set of the compatible subgraph G.
 * Returns |M| + alpha(G); the full set is written to fullout. */
static int extend(int *M, int m, int *fullout) {
    uint64_t forb[W];
    memset(forb, 0, sizeof(forb));
    for (int i = 0; i < m; i++) {
        int w = M[i];
        for (int j = 0; j < 243; j++) { int u = nbr[w][j]; forb[u / 64] |= 1ULL << (u % 64); }
    }
    mis_clear();
    int g = 0;
    for (int u = 0; u < NV; u++) {
        if (forb[u / 64] >> (u % 64) & 1ULL) continue;
        idxmap[u] = g++;
        if (g >= MAXG) return -1;
    }
    if (g == 0) { for (int i = 0; i < m; i++) fullout[i] = M[i]; return m; }
    uint64_t best[GW];
    double t0 = now_s();
    int a;
    if (g > 120) {
        /* Large candidate graph: a greedy maximal independent set via neighbour
         * marking.  The exact MIS here is exponential and used to dominate the
         * whole search (tens of seconds per evaluation).  The exact path is
         * kept for small graphs, which is where the optimum matters. */
        uint64_t blocked[W];
        memset(blocked, 0, sizeof(blocked));
        memset(best, 0, sizeof(uint64_t) * GW);
        int cnt = 0;
        for (int u = 0; u < NV; u++) {
            if (forb[u / 64] >> (u % 64) & 1ULL) continue;
            if (blocked[u / 64] >> (u % 64) & 1ULL) continue;
            int iu = idxmap[u];
            best[iu / 64] |= 1ULL << (iu % 64);
            cnt++;
            for (int j = 0; j < 243; j++) {
                int x = nbr[u][j];
                blocked[x / 64] |= 1ULL << (x % 64);
            }
        }
        a = cnt;
        /* emit the chosen points (they are graph vertices, not M indices) */
        {
            uint64_t sel[W];
            memset(sel, 0, sizeof(sel));
            for (int u = 0; u < NV; u++) {
                if (forb[u / 64] >> (u % 64) & 1ULL) continue;
                int iu = idxmap[u];
                if (best[iu / 64] >> (iu % 64) & 1ULL) sel[u / 64] |= 1ULL << (u % 64);
            }
            int c = 0;
            for (int i = 0; i < m; i++) fullout[c++] = M[i];
            for (int u = 0; u < NV; u++)
                if (sel[u / 64] >> (u % 64) & 1ULL) fullout[c++] = u;
            if (g_dbg_verbose) fprintf(stderr, "   extend(greedy): |G|=%d alpha=%d\n", g, a);
            return c;
        }
    }
    memset(gadj, 0, sizeof(uint64_t) * GW * g);
    for (int u = 0; u < NV; u++) {
        if (forb[u / 64] >> (u % 64) & 1ULL) continue;
        int iu = idxmap[u];
        gm[iu / 64] |= 1ULL << (iu % 64);
        for (int j = 0; j < 243; j++) {
            int x = nbr[u][j];
            if (forb[x / 64] >> (x % 64) & 1ULL) continue;
            int ix = idxmap[x];
            gadj[iu][ix / 64] |= 1ULL << (ix % 64);
        }
    }
    gn = g;
    double t0b = now_s();
    a = alpha_exact(best);
    double t1 = now_s();
    if (g_dbg_verbose) fprintf(stderr, "   extend: |G|=%d alpha=%d nodes=%ld time=%.3fs\n", g, a, g_nodes, t1 - t0);
    if (a < 0) return -1;
    int c = 0;
    for (int i = 0; i < m; i++) fullout[c++] = M[i];
    for (int u = 0; u < NV; u++) {
        if (forb[u / 64] >> (u % 64) & 1ULL) continue;
        int iu = idxmap[u];
        if (best[iu / 64] >> (iu % 64) & 1ULL) fullout[c++] = u;
    }
    return c;
}

static int mcount, gsize, alpha_g;
/* generalised rounding map: per-coordinate, NON-UNIFORM partitions of [0,n)
 * into 7 cells.  The Polak-Schrijver construction only ever used one
 * uniform division factor; this is the unexplored generalisation. */
static int   g_use_bounds = 0;
static int   g_bounds_k = 0;
static int   g_bounds[DIM][7];
static long  g_bounds_n = 0;

static inline int cell_of(long z, const double fv_k) {
    if (g_use_bounds) {
        int lo = 0, hi = 6;
        while (lo < hi) { int mid = (lo + hi + 1) >> 1; if (z >= g_bounds[g_bounds_k][mid]) lo = mid; else hi = mid - 1; }
        return lo;
    }
    int b = (int)floor((double)z / fv_k);
    if (b < 0) b = 0;
    if (b > 6) b = 6;
    return b;
}
static int g_dbg_m1, g_dbg_m2, g_dbg_t1, g_dbg_t2;
static int g_use_rule_b = 1;

/* evaluate one parameter tuple; returns the best total over the two rules
 * for step (iv).  The full set is written to fullout. */
static int evaluate_f(long n, const long v[DIM], const long off[DIM], const double fv[DIM], int *fullout) {
    int Wlist[4096];
    int nw = 0;
    long cur[DIM];
    for (int k = 0; k < DIM; k++) cur[k] = ((off[k] % n) + n) % n;
    for (long t = 0; t < n; t++) {
        int idx = 0;
        for (int k = 0; k < DIM; k++) {
            long c = cur[k] % n;
            if (c < 0) c += n;
            g_bounds_k = k;
            int b = cell_of(c, fv[k]);
            idx = idx * CYC + b;
        }
        if (nw < 4096) Wlist[nw++] = idx;
        for (int k = 0; k < DIM; k++) {
            cur[k] = (cur[k] + v[k]) % n;
            if (cur[k] < 0) cur[k] += n;
        }
    }
    memset(occ, 0, sizeof(occ));
    memset(cntN, 0, sizeof(cntN));
    for (int i = 0; i < nw; i++) occ[Wlist[i]]++;
    for (int i = 0; i < nw; i++) {
        int w = Wlist[i];
        for (int j = 0; j < 243; j++) cntN[nbr[w][j]]++;
    }
    /* rule A: the paper's rule (drop every word that touches another) */
    int M1[4096], m1 = 0;
    memset(occ, 0, sizeof(occ));
    for (int i = 0; i < nw; i++) {
        int w = Wlist[i];
        if (occ[w]) continue;
        occ[w] = 1;
        if (cntN[w] == 1) M1[m1++] = w;
    }
    /* rule B: exact maximum independent set of the conflict graph */
    int M2[4096];
    int m2 = -1;
    if (g_use_rule_b) {
        m2 = conflict_mis(Wlist, nw, M2);
        if (m2 < 0) return -1;
    }

    int best = -1, bestm = 0;
    int buf[4096];
    int t1 = extend(M1, m1, buf);
    if (t1 > best) { best = t1; bestm = m1; memcpy(fullout, buf, sizeof(int) * t1); }
    int t2 = -1;
    if (m2 >= 0 && m2 != m1) {
        t2 = extend(M2, m2, buf);
        if (t2 > best) { best = t2; bestm = m2; memcpy(fullout, buf, sizeof(int) * t2); }
    }
    g_dbg_m1 = m1; g_dbg_m2 = m2; g_dbg_t1 = t1; g_dbg_t2 = t2;
    mcount = bestm;
    alpha_g = best - bestm;
    gsize = best;
    return best;
}

/* uniform-division-factor wrapper */
static int evaluate(long n, const long v[DIM], const long off[DIM], double f, int *fullout) {
    double fv[DIM];
    for (int k = 0; k < DIM; k++) fv[k] = f;
    return evaluate_f(n, v, off, fv, fullout);
}

/* minimum distance of the cyclic code { t*v } in Z_n^5 */
static long code_distance(long n, const long v[DIM]) {
    long best = n;
    for (long t = 1; t < n; t++) {
        long mx = 0;
        for (int k = 0; k < DIM; k++) {
            long c = (t * v[k]) % n;
            if (c < 0) c += n;
            long d = c < n - c ? c : n - c;
            if (d > mx) mx = d;
            if (mx >= best) break;
        }
        if (mx < best) best = mx;
        if (best <= 1) break;
    }
    return best;
}

/* ------------------------------------------------------------------ main -- */

static int best_total = 0;
static long best_n, best_q, best_off[DIM];
static double best_f;
static int  best_words[4096], best_nwords;

static void report(const char *tag, long n, long q, const long v[DIM], const long off[DIM], double f,
                   int total, int m, int g) {
    printf("%s n=%ld q=%ld off=(%ld,%ld,%ld,%ld,%ld) f=%g v=(%ld,%ld,%ld,%ld,%ld) |M|=%d total=%d  [A:%d/%d B:%d/%d]\n",
           tag, n, q, off[0], off[1], off[2], off[3], off[4], f,
           v[0], v[1], v[2], v[3], v[4], m, total, g_dbg_m1, g_dbg_t1, g_dbg_m2, g_dbg_t2);
    fflush(stdout);
}

static void dump_words(const char *path, int *w, int n) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < n; i++) {
        int v = w[i], buf[5];
        for (int k = 4; k >= 0; k--) { buf[k] = v % CYC; v /= CYC; }
        fprintf(f, "%d%d%d%d%d\n", buf[0], buf[1], buf[2], buf[3], buf[4]);
    }
    fclose(f);
}

/* hill-climb on the intermediate set M, maximising |M| + alpha(G(M)).
 * Works for an arbitrary base code v (not just the geometric family). */
static int polish_run(long n, const long v[DIM], const long off[DIM], double f) {

        /* hill-climb on the intermediate set M, maximising |M| + alpha(G(M)) */
        int words[4096];
        int T;
        int Wlist[4096], nw = 0;
        long cur[DIM];
        for (int k = 0; k < DIM; k++) cur[k] = ((off[k] % n) + n) % n;
        for (long t = 0; t < n; t++) {
            int idx = 0;
            for (int k = 0; k < DIM; k++) {
                long c = cur[k] % n; if (c < 0) c += n;
                int b = (int)floor((double)c / f);
                if (b < 0) b = 0; if (b > 6) b = 6;
                idx = idx * CYC + b;
            }
            Wlist[nw++] = idx;
            for (int k = 0; k < DIM; k++) { cur[k] = (cur[k] + v[k]) % n; if (cur[k] < 0) cur[k] += n; }
        }
        memset(occ, 0, sizeof(occ));
        memset(cntN, 0, sizeof(cntN));
        for (int i = 0; i < nw; i++) occ[Wlist[i]]++;
        for (int i = 0; i < nw; i++) { int w = Wlist[i]; for (int j = 0; j < 243; j++) cntN[nbr[w][j]]++; }
        int M[4096], m = 0;
        memset(occ, 0, sizeof(occ));
        for (int i = 0; i < nw; i++) { int w = Wlist[i]; if (occ[w]) continue; occ[w] = 1; if (cntN[w] == 1) M[m++] = w; }
        T = extend(M, m, words);
        if (T > best_total) { best_total = T; best_nwords = T; memcpy(best_words, words, sizeof(int) * T); }
        fprintf(stderr, "start |M|=%d T=%d\n", m, T);
        for (int iter = 0; iter < 60; iter++) {
            int bestT = T, bestu = -1;
            int buf[4096];
            for (int i = 0; i < m; i++) {
                int M2[4096];
                int m2 = 0;
                for (int j = 0; j < m; j++) if (j != i) M2[m2++] = M[j];
                int t2 = extend(M2, m2, buf);
                if (t2 > bestT) { bestT = t2; bestu = i; }
            }
            if (bestu < 0) { fprintf(stderr, "local optimum: T=%d |M|=%d\n", T, m); break; }
            for (int j = bestu, k2 = j; k2 < m - 1; k2++) M[k2] = M[k2 + 1];
            m--;
            T = bestT;
            T = extend(M, m, words);
            fprintf(stderr, "iter %d: |M|=%d T=%d\n", iter, m, T);
            if (T > best_total) {
                best_total = T; best_nwords = T;
                memcpy(best_words, words, sizeof(int) * T);
                dump_words("best5.txt", best_words, T);
                fprintf(stderr, "  ** new best %d written to best5.txt\n", T);
            }
        }
                return best_total;
    }


/* ------------------------------------------------------------------ */
/* Local search directly on the intermediate set M, maximising          */
/*     |M| + alpha(G(M))                                              */
/* Moves: remove one word / swap in a word (dropping its conflicts).  */
/* The Polak-Schrijver paper only tried a "drop 3, add 4" swap; this  */
/* searches the whole neighbourhood.                                   */
/* ------------------------------------------------------------------ */
static int ls_run(long n, const long v[DIM], const long off[DIM], double f, double budget) {
    int Wlist[4096], nw = 0;
    long cur[DIM];
    for (int k = 0; k < DIM; k++) cur[k] = ((off[k] % n) + n) % n;
    for (long t = 0; t < n; t++) {
        int idx = 0;
        for (int k = 0; k < DIM; k++) {
            long c = cur[k] % n; if (c < 0) c += n;
            int b = (int)floor((double)c / f);
            if (b < 0) b = 0; if (b > 6) b = 6;
            idx = idx * CYC + b;
        }
        Wlist[nw++] = idx;
        for (int k = 0; k < DIM; k++) { cur[k] = (cur[k] + v[k]) % n; if (cur[k] < 0) cur[k] += n; }
    }
    /* dedupe the image */
    int uniq[4096]; int nu = 0;
    memset(occ, 0, sizeof(occ));
    for (int i = 0; i < nw; i++) { if (!occ[Wlist[i]]) { occ[Wlist[i]] = 1; uniq[nu++] = Wlist[i]; } }

    /* conflict relation over the image */
    int confl[4096][243];
    static uint8_t inw[NV];
    memset(inw, 0, sizeof(inw));
    for (int i = 0; i < nu; i++) inw[uniq[i]] = 1;
    for (int i = 0; i < nu; i++) {
        int c = 0;
        for (int j = 0; j < 243; j++) { int x = nbr[uniq[i]][j]; if (inw[x]) confl[i][c++] = x; }
    }
    /* index of each image point */
    static int widx[NV];
    memset(widx, -1, sizeof(widx));
    for (int i = 0; i < nu; i++) widx[uniq[i]] = i;

    /* start from the paper's rule A: keep only conflict-free words */
    int M[4096], m = 0;
    memset(occ, 0, sizeof(occ));
    for (int i = 0; i < nu; i++) {
        int w = uniq[i];
        int ok = 1;
        for (int j = 0; j < 243; j++) { int x = nbr[w][j]; if (x != w && inw[x] && occ[x]) { ok = 0; break; } }
        if (!ok) continue;
        occ[w] = 1; M[m++] = w;
    }
    int words[4096];
    int T = extend(M, m, words);
    int bestT = T, bestm = m;
    int bestM[4096]; memcpy(bestM, M, sizeof(int) * m);
    fprintf(stderr, "ls start |M|=%d T=%d (image %d)\n", m, T, nu);

    double t0 = now_s();
    while (now_s() - t0 < budget) {
        int bestMoveT = T, bestMoveType = -1, bestMoveIdx = -1;
        int buf[4096];
        /* move 1: remove one word */
        for (int i = 0; i < m; i++) {
            int M2[4096]; int m2 = 0;
            for (int j = 0; j < m; j++) if (j != i) M2[m2++] = M[j];
            int t2 = extend(M2, m2, buf);
            if (t2 > bestMoveT) { bestMoveT = t2; bestMoveType = 0; bestMoveIdx = i; }
        }
        /* move 2: swap in a word that is currently outside M */
        for (int i = 0; i < nu; i++) {
            int w = uniq[i];
            if (occ[w]) continue;
            /* which words of M conflict with w ? */
            int M2[4096]; int m2 = 0;
            for (int j = 0; j < m; j++) {
                int conflict = 0;
                for (int q = 0; q < 243; q++) if (nbr[w][q] == M[j]) { conflict = 1; break; }
                if (!conflict) M2[m2++] = M[j];
            }
            M2[m2++] = w;
            int t2 = extend(M2, m2, buf);
            if (t2 > bestMoveT) { bestMoveT = t2; bestMoveType = 1; bestMoveIdx = i; }
        }
        if (bestMoveType < 0) { fprintf(stderr, "ls local optimum |M|=%d T=%d\n", m, T); break; }
        if (bestMoveType == 0) {
            for (int j = bestMoveIdx, k2 = j; k2 < m - 1; k2++) M[k2] = M[k2 + 1];
            m--;
            memset(occ, 0, sizeof(occ));
            for (int j = 0; j < m; j++) occ[M[j]] = 1;
        } else {
            int w = uniq[bestMoveIdx];
            int M2[4096]; int m2 = 0;
            for (int j = 0; j < m; j++) {
                int conflict = 0;
                for (int q = 0; q < 243; q++) if (nbr[w][q] == M[j]) { conflict = 1; break; }
                if (!conflict) M2[m2++] = M[j];
            }
            M2[m2++] = w;
            memcpy(M, M2, sizeof(int) * m2); m = m2;
            memset(occ, 0, sizeof(occ));
            for (int j = 0; j < m; j++) occ[M[j]] = 1;
        }
        T = extend(M, m, words);
        if (T > bestT) {
            bestT = T; bestm = m;
            memcpy(bestM, M, sizeof(int) * m);
            fprintf(stderr, "ls: |M|=%d T=%d  ** best\n", m, T);
        }
        fprintf(stderr, "ls move type=%d |M|=%d T=%d\n", bestMoveType, m, T);
    }
    int words2[4096];
    int t2 = extend(bestM, bestm, words2);
    if (t2 > bestT) { bestT = t2; memcpy(best_words, words2, sizeof(int) * t2); best_nwords = t2; }
    else { memcpy(best_words, words, sizeof(int) * bestT); best_nwords = bestT; }
    dump_words("ls_best.txt", best_words, best_nwords);
    fprintf(stderr, "ls done: best T=%d |M|=%d\n", bestT, bestm);
    return bestT;
}


/* ------------------------------------------------------------------ */
/* Hill-climb on (constant word, division factors) for a FIXED code v, */
/* maximising |M| + alpha(G(M)).  Layer 2 of the search:              */
/*   layer 1  vsearch : random (v, off, f) samples                      */
/*   layer 2  vhc     : hill-climb (off, f) for one code                */
/*   layer 3  ls      : local search on the word set M itself          */
/* ------------------------------------------------------------------ */
static int vhc_run(long n, const long v[DIM], double budget, uint64_t seed, int percoord,
                   const long *start_off, const double *start_f) {
    rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    if (!rs) rs = 88172645463326252ULL;
    g_use_rule_b = 0;                     /* the paper's rule A packs better */
    g_node_budget = 5000;
    long off[DIM];
    double fv[DIM];
    for (int k = 0; k < DIM; k++) {
        off[k] = start_off ? start_off[k] : (long)(rnd() % (uint64_t)n);
        fv[k] = start_f ? start_f[k] : (double)n / 7.0;
    }
    int words[4096];
    int T = evaluate_f(n, v, off, fv, words);
    long boff[DIM]; double bfv[DIM]; int bestT = T;
    memcpy(boff, off, sizeof(off)); memcpy(bfv, fv, sizeof(fv));
    fprintf(stderr, "vhc start n=%ld T=%d\n", n, T);
    double t0 = now_s();
    int it = 0;
    while (now_s() - t0 < budget) {
        it++;
        long o2[DIM]; double f2[DIM];
        memcpy(o2, boff, sizeof(o2)); memcpy(f2, bfv, sizeof(f2));
        int mv = (int)(rnd() % 3ULL);
        if (mv == 0) {                          /* shift one constant */
            int j = (int)(rnd() % DIM);
            long d = (long)(rnd() % (uint64_t)n);
            o2[j] = (o2[j] + d) % n;
        } else if (mv == 1) {                   /* perturb one division factor */
            int j = (int)(rnd() % DIM);
            f2[j] += ((double)(rnd() % 2001) / 1000.0 - 1.0);
            if (f2[j] < 1.0) f2[j] = 1.0;
        } else {                                /* perturb all factors slightly */
            for (int j = 0; j < DIM; j++) {
                f2[j] += ((double)(rnd() % 401) / 1000.0 - 0.2);
                if (f2[j] < 1.0) f2[j] = 1.0;
            }
        }
        (void)percoord;
        int t2 = evaluate_f(n, v, o2, f2, words);
        if (t2 < 0) continue;                   /* MIS budget hit: skip */
        if (t2 >= bestT) {                     /* hill climb, accept ties */
            bestT = t2;
            memcpy(boff, o2, sizeof(o2)); memcpy(bfv, f2, sizeof(f2));
            if (t2 > T) { T = t2; fprintf(stderr, "vhc T=%d off=(%ld,%ld,%ld,%ld,%ld) f=%.3f\n", T, boff[0], boff[1], boff[2], boff[3], boff[4], bfv[0]); }
        }
        if (it % 200 == 0) fprintf(stderr, "vhc %d iters, best=%d, %.0fs\n", it, bestT, now_s() - t0);
    }
    printf("VHC n=%ld best=%d off=(%ld,%ld,%ld,%ld,%ld) f=(%.4f,%.4f,%.4f,%.4f,%.4f)\n",
           n, bestT, boff[0], boff[1], boff[2], boff[3], boff[4], bfv[0], bfv[1], bfv[2], bfv[3], bfv[4]);
    fflush(stdout);
    FILE *rf = fopen("vhc_results.txt", "a");
    if (rf) {
        fprintf(rf, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %.4f %.4f %.4f %.4f %.4f\n",
                (long)bestT, n, v[0], v[1], v[2], v[3], v[4], boff[0], boff[1], boff[2], boff[3], boff[4],
                bfv[0], bfv[1], bfv[2], bfv[3], bfv[4]);
        fclose(rf);
    }
    return bestT;
}

int main(int argc, char **argv) {
    gen_graph();
    const char *mode = argc > 1 ? argv[1] : "one";
    g_node_budget = 200000;

    if (!strcmp(mode, "one")) {
        long n = atol(argv[2]), q = atol(argv[3]);
        long off[DIM], v[DIM];
        for (int k = 0; k < DIM; k++) off[k] = atol(argv[4 + k]);
        double f = argc > 9 ? atof(argv[9]) : (double)n / 7.0;
        long pw = 1;
        for (int k = 0; k < DIM; k++) { v[k] = pw % n; pw = (pw * q) % n; }
        long k = code_distance(n, v);
        if (argc > 10 && !strcmp(argv[10], "A")) g_use_rule_b = 0;
        if (argc > 10 && !strcmp(argv[10], "V")) { g_use_rule_b = 0; g_dbg_verbose = 1; }
        int words[4096];
        int total = evaluate(n, v, off, f, words);
        printf("n=%ld q=%ld k=%ld n/k=%.4f |M|=%d total=%d   [ruleA: m=%d tot=%d | ruleB: m=%d tot=%d]\n",
               n, q, k, (double)n / k, mcount, total, g_dbg_m1, g_dbg_t1, g_dbg_m2, g_dbg_t2);
        if (total > 0) dump_words(getenv("OUT5") ? getenv("OUT5") : "out5.txt", words, total);
        return 0;
    }

    if (!strcmp(mode, "vsearch")) {
        /* vsearch <codesfile> <seed> <secs> <flags>
         * Joint search over: base code (v), constant word (off), division
         * factor(s) f.  The Polak-Schrijver paper only ever varied off and f
         * for ONE fixed geometric code; here the code varies too.
         * flags bit0: per-coordinate division factors
         * flags bit1: also try the paper-style alternating constant pattern
         */
        const char *file = argv[2];
        uint64_t seed = strtoull(argv[3], NULL, 10);
        double budget = atof(argv[4]);
        int flags = argc > 5 ? atoi(argv[5]) : 0;
        g_node_budget = 5000;            /* keep each evaluation fast */
        g_use_rule_b = 0;                /* the paper's rule A: faster AND better */
        rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        if (!rs) rs = 88172645463326252ULL;

        long cn = 0, cv[4096][5], ck[4096], cnof[4096];
        FILE *fp = fopen(file, "r");
        if (!fp) { perror("open codes"); return 1; }
        char line[256];
        while (fgets(line, sizeof(line), fp) && cn < 4096) {
            long n, a0, a1, a2, a3, a4, k;
            if (sscanf(line, "%ld %ld %ld %ld %ld %ld %ld", &n, &a0, &a1, &a2, &a3, &a4, &k) != 7) {
                if (sscanf(line, "%ld %ld %ld %ld %ld %ld", &n, &a0, &a1, &a2, &a3, &a4) != 6) continue;
                k = 0;
            }
            cv[cn][0] = a0; cv[cn][1] = a1; cv[cn][2] = a2; cv[cn][3] = a3; cv[cn][4] = a4; ck[cn] = k; cnof[cn] = n; cn++;
        }
        fclose(fp);
        if (!cn) { printf("no codes\n"); return 1; }
        printf("vsearch: %ld codes loaded, seed=%llu budget=%.0fs flags=%d\n",
               cn, (unsigned long long)seed, budget, flags);

        int words[4096];
        int best = 0, bestm = 0;
        long bestn = 0, bestoff[DIM]; double bestf[DIM];
        long bestv[DIM];
        long nev = 0;
        double t_start = now_s();
        while (now_s() - t_start < budget) {
            long ci = (long)(rnd() % (uint64_t)cn);
            long n = cnof[ci];
            const long *v = cv[ci];
            long off[DIM];
            int pat = (flags & 2) ? (int)(rnd() & 1ULL) : (int)(rnd() % 2ULL);
            if (pat == 0) {
                long a = (long)(rnd() % (uint64_t)n), b = (long)(rnd() % (uint64_t)n);
                off[0] = a; off[1] = b; off[2] = a; off[3] = b; off[4] = a;
            } else {
                for (int j = 0; j < DIM; j++) off[j] = (long)(rnd() % (uint64_t)n);
            }
            double fv[DIM];
            if (flags & 1) {
                for (int j = 0; j < DIM; j++)
                    fv[j] = (double)n / 7.0 + ((double)(rnd() % 4001) / 1000.0 - 2.0);
            } else {
                double f = (double)n / 7.0 + ((double)(rnd() % 4001) / 1000.0 - 2.0);
                for (int j = 0; j < DIM; j++) fv[j] = f;
            }
            double te0 = now_s();
            int T = evaluate_f(n, v, off, fv, words);
            double te1 = now_s();
            if (T < 0) { nev++; continue; }
            nev++;
            if (te1 - te0 > 0.5) fprintf(stderr, "  SLOW eval #%ld took %.2fs (n=%ld g=%d)\n", nev, te1 - te0, n, gsize);
            if (nev % 20 == 0) fprintf(stderr, "  ... %ld evals, best=%d, elapsed=%.0fs, last=%.3fs\n", nev, best, now_s() - t_start, te1 - te0);
            if (T > best) {
                best = T; bestm = mcount;
                bestn = n;
                for (int j = 0; j < DIM; j++) { bestoff[j] = off[j]; bestf[j] = fv[j]; bestv[j] = v[j]; }
                printf("NEW BEST total=%d |M|=%d n=%ld v=(%ld,%ld,%ld,%ld,%ld) off=(%ld,%ld,%ld,%ld,%ld) f=%.3f\n",
                       T, bestm, n, v[0], v[1], v[2], v[3], v[4], off[0], off[1], off[2], off[3], off[4], fv[0]);
                fflush(stdout);
                char path[256];
                snprintf(path, sizeof(path), "vs_best_%d.txt", (int)seed);
                dump_words(path, words, T);
                FILE *rf = fopen("vs_results.txt", "a");
                if (rf) {
                    fprintf(rf, "%ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %.4f %.4f %.4f %.4f %.4f\n",
                            (long)T, (long)bestm, (long)n, (long)v[0], (long)v[1], (long)v[2], (long)v[3], (long)v[4],
                            (long)off[0], (long)off[1], (long)off[2], (long)off[3], (long)off[4],
                            fv[0], fv[1], fv[2], fv[3], fv[4]);
                    fclose(rf);
                }
            }
        }
        printf("vsearch done: %ld evaluations, best total=%d (|M|=%d) n=%ld\n", nev, best, bestm, bestn);
        return 0;
    }

    if (!strcmp(mode, "vcode")) {
        /* vcode n v0..v4 off0..off4 [f] : an arbitrary code generator */
        long n = atol(argv[2]);
        long v[DIM], off[DIM];
        for (int k = 0; k < DIM; k++) v[k] = atol(argv[3 + k]);
        for (int k = 0; k < DIM; k++) off[k] = atol(argv[8 + k]);
        double f = argc > 13 ? atof(argv[13]) : (double)n / 7.0;
        long k = code_distance(n, v);
        int words[4096];
        int total = evaluate(n, v, off, f, words);
        printf("n=%ld v=(%ld,%ld,%ld,%ld,%ld) k=%ld n/k=%.4f |M|=%d total=%d\n",
               n, v[0], v[1], v[2], v[3], v[4], k, (double)n / k, mcount, total);
        if (total > 0) dump_words(getenv("OUT5") ? getenv("OUT5") : "out5.txt", words, total);
        return 0;
    }

    if (!strcmp(mode, "list")) {
        long nmin = atol(argv[2]), nmax = atol(argv[3]);
        long qmin = atol(argv[4]), qmax = atol(argv[5]);
        double lo = argc > 6 ? atof(argv[6]) : 3.0;
        double hi = argc > 7 ? atof(argv[7]) : 3.6;
        for (long n = nmin; n <= nmax; n++) {
            for (long q = qmin; q <= qmax && q < n; q++) {
                long v[DIM], pw = 1;
                for (int k2 = 0; k2 < DIM; k2++) { v[k2] = pw % n; pw = (pw * q) % n; }
                long k = code_distance(n, v);
                if (k < 2) continue;
                double ratio = (double)n / k;
                if (ratio < lo || ratio > hi) continue;
                printf("%ld %ld k=%ld ratio=%.4f\n", n, q, k, ratio);
            }
        }
        return 0;
    }

    if (!strcmp(mode, "sweep")) {
        long nmin = atol(argv[2]), nmax = atol(argv[3]);
        long qmin = atol(argv[4]), qmax = atol(argv[5]);
        uint64_t seed = strtoull(argv[6], NULL, 10);
        long noff = atol(argv[7]);
        int pat = argc > 8 ? atoi(argv[8]) : 0;
        rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        if (!rs) rs = 88172645463325252ULL;
        if (argc > 9 && !strcmp(argv[9], "A")) g_use_rule_b = 0;
        long nfound = 0, nev = 0;
        int words[4096];
        for (long n = nmin; n <= nmax; n++) {
            for (long q = qmin; q <= qmax && q < n; q++) {
                long v[DIM], pw = 1;
                for (int k = 0; k < DIM; k++) { v[k] = pw % n; pw = (pw * q) % n; }
                long k = code_distance(n, v);
                if (k < 2) continue;
                double ratio = (double)n / k;
                if (ratio < 3.0 || ratio > 3.75) continue;
                nfound++;
                for (long t = 0; t < noff; t++) {
                    long off[DIM];
                    if (pat == 0) {
                        long a = (long)(rnd() % (uint64_t)n), b = (long)(rnd() % (uint64_t)n);
                        off[0] = a; off[1] = b; off[2] = a; off[3] = b; off[4] = a;
                    } else if (pat == 2) {
                        off[0] = off[2] = off[4] = (long)(rnd() % (uint64_t)n);
                        off[1] = off[3] = (long)(rnd() % (uint64_t)n);
                        off[0] = 40; off[1] = 123; off[2] = 40; off[3] = 123; off[4] = 40;
                    } else {
                        for (int j = 0; j < DIM; j++) off[j] = (long)(rnd() % (uint64_t)n);
                    }
                    double f = (double)n / 7.0;
                    int total = evaluate(n, v, off, f, words);
                    nev++;
                    if (total > best_total) {
                        best_total = total;
                        best_n = n; best_q = q; best_f = f;
                        for (int j = 0; j < DIM; j++) best_off[j] = off[j];
                        memcpy(best_words, words, sizeof(int) * total);
                        best_nwords = total;
                        report("BEST", n, q, v, off, f, total, mcount, gsize);
                        dump_words(getenv("OUT5") ? getenv("OUT5") : "best5.txt", best_words, best_nwords);
                    }
                }
            }
        }
        printf("sweep done: evaluated %ld offsets over %ld (n,q) pairs; best total %d\n", nev, nfound, best_total);
        if (best_total)
            printf("best: n=%ld q=%ld off=(%ld,%ld,%ld,%ld,%ld) f=%g TOTAL=%d\n",
                   best_n, best_q, best_off[0], best_off[1], best_off[2], best_off[3], best_off[4], best_f, best_total);
        return 0;
    }

    if (!strcmp(mode, "polish")) {
        long n = atol(argv[2]), q = atol(argv[3]);
        long off[DIM], v[DIM];
        for (int k = 0; k < DIM; k++) off[k] = atol(argv[4 + k]);
        double f = argc > 9 ? atof(argv[9]) : (double)n / 7.0;
        long pw = 1;
        for (int k = 0; k < DIM; k++) { v[k] = pw % n; pw = (pw * q) % n; }
        int r = polish_run(n, v, off, f);
        printf("polish done: best %d\n", r);
        return 0;
    }

    if (!strcmp(mode, "vpb")) {
        /* vpb n v0..v4 <secs> <seed> : hill-climb the per-coordinate partitions */
        long n = atol(argv[2]);
        long v[DIM];
        for (int k = 0; k < DIM; k++) v[k] = atol(argv[3 + k]);
        double budget = argc > 8 ? atof(argv[8]) : 60.0;
        uint64_t seed = argc > 9 ? strtoull(argv[9], NULL, 10) : 12345;
        rs = seed * 6364136224006793005ULL + 1442695040888963407ULL;
        if (!rs) rs = 88172645463325252ULL;
        g_use_rule_b = 0;
        g_node_budget = 5000;
        g_use_bounds = 1; g_bounds_n = n;
        /* uniform start */
        for (int k = 0; k < DIM; k++)
            for (int j = 0; j < 7; j++)
                g_bounds[k][j] = (int)((long long)j * n / 7);
        long off[DIM]; double fv[DIM];
        for (int k = 0; k < DIM; k++) { off[k] = (long)(rnd() % (uint64_t)n); fv[k] = (double)n / 7.0; }
        if (argc > 15) for (int k = 0; k < DIM; k++) off[k] = atol(argv[11 + k]);
        if (argc > 16) {
            /* optional warm start: a file with 5 lines of 7 boundaries each */
            FILE *bf = fopen(argv[16], "r");
            if (bf) {
                for (int k = 0; k < DIM; k++) {
                    int b[7];
                    if (fscanf(bf, "%d %d %d %d %d %d %d", &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6]) == 7)
                        for (int j = 0; j < 7; j++) g_bounds[k][j] = b[j];
                }
                fclose(bf);
                fprintf(stderr, "vpb: warm-start boundaries loaded\n");
            }
        }
        int words[4096];
        int T = evaluate_f(n, v, off, fv, words);
        int bestT = T; int bestB[DIM][7]; long boff[DIM];
        memcpy(bestB, g_bounds, sizeof(g_bounds));
        memcpy(boff, off, sizeof(off));
        fprintf(stderr, "vpb start n=%ld T=%d\n", n, T);
        double t0 = now_s(); long it = 0;
        while (now_s() - t0 < budget) {
            it++;
            memcpy(g_bounds, bestB, sizeof(g_bounds));
            long o2[DIM]; memcpy(o2, boff, sizeof(o2));
            int mv = (int)(rnd() % 4ULL);
            if (mv == 3) {
                /* shift one constant word */
                int k = (int)(rnd() % DIM);
                o2[k] = (o2[k] + (long)(rnd() % (uint64_t)n)) % n;
            } else {
                int k = (int)(rnd() % DIM);
                int j = 1 + (int)(rnd() % 5);
                long delta = (long)(rnd() % 11) - 5;
                int nv = g_bounds[k][j] + (int)delta;
                if (nv <= g_bounds[k][j-1] || nv >= g_bounds[k][j+1]) continue;
                g_bounds[k][j] = nv;
            }
            int t2 = evaluate_f(n, v, o2, fv, words);
            if (t2 < 0) continue;          /* MIS budget hit: skip this sample */
            if (t2 >= bestT) {
                bestT = t2;
                memcpy(bestB, g_bounds, sizeof(g_bounds));
                memcpy(boff, o2, sizeof(o2));
                if (t2 > T) { T = t2; fprintf(stderr, "vpb T=%d\n", T); fflush(stderr); }
            }
            if (it % 500 == 0) fprintf(stderr, "vpb %ld iters best=%d %.0fs\n", it, bestT, now_s() - t0);
            /* Periodic restart: go back to the BEST boundaries and perturb a
             * few of them.  A fully random restart throws away the good basin
             * (the Polak-Schrijver partition) and the search never comes back. */
            if (now_s() - t0 > (double)(it / 1500 + 1) * 25.0) {
                memcpy(g_bounds, bestB, sizeof(g_bounds));
                for (int z = 0; z < 3; z++) {
                    int k = (int)(rnd() % DIM);
                    int j = 1 + (int)(rnd() % 5);
                    int nv = g_bounds[k][j] + (int)(rnd() % 7) - 3;
                    if (nv > g_bounds[k][j-1] && nv < g_bounds[k][j+1]) g_bounds[k][j] = nv;
                }
            }
        }
        printf("VPB n=%ld best=%d off=(%ld,%ld,%ld,%ld,%ld) bnd=", n, bestT, boff[0], boff[1], boff[2], boff[3], boff[4]);
        for (int k = 0; k < DIM; k++) { for (int j = 0; j < 7; j++) printf("%d,", bestB[k][j]); printf("|"); }
        printf("\n");
        fflush(stdout);
        FILE *rf = fopen("vpb_results.txt", "a");
        if (rf) { fprintf(rf, "%d %ld %ld %ld %ld %ld %ld\n", bestT, n, v[0], v[1], v[2], v[3], v[4]); fclose(rf); }
        return 0;
    }

    if (!strcmp(mode, "vj")) {
        /* vj n v0..v4 <secs> <seed> [pc] [off0..4] [bndfile]
         * Joint hill-climb over BOTH the constant word (off) and the
         * per-coordinate non-uniform partition boundaries, starting from the
         * Polak-Schrijver parameters (367 points).  The vpb mode only moved
         * the boundaries, vhc only the constant word; the 367 point record
         * is a sharp local optimum in the joint space, so both must move
         * together to escape it. */
        long n = atol(argv[2]);
        long v[DIM];
        for (int k = 0; k < DIM; k++) v[k] = atol(argv[3 + k]);
        double budget = argc > 8 ? atof(argv[8]) : 60.0;
        uint64_t seed = argc > 9 ? strtoull(argv[9], NULL, 10) : 12345;
        rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        if (!rs) rs = 88172645463325252ULL;
        g_use_rule_b = 0;
        g_node_budget = 5000;
        g_use_bounds = 1; g_bounds_n = n;
        for (int k = 0; k < DIM; k++)
            for (int j = 0; j < 7; j++)
                g_bounds[k][j] = (int)((long long)j * n / 7);
        long off[DIM]; double fv[DIM];
        for (int k = 0; k < DIM; k++) { off[k] = (long)(rnd() % (uint64_t)n); fv[k] = (double)n / 7.0; }
        if (argc > 15) for (int k = 0; k < DIM; k++) off[k] = atol(argv[11 + k]);
        if (argc > 16) {
            FILE *bf = fopen(argv[16], "r");
            if (bf) {
                for (int k = 0; k < DIM; k++) {
                    int b[7];
                    if (fscanf(bf, "%d %d %d %d %d %d %d", &b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6]) == 7)
                        for (int j = 0; j < 7; j++) g_bounds[k][j] = b[j];
                }
                fclose(bf);
            }
        }
        int words[4096];
        int T = evaluate_f(n, v, off, fv, words);
        int bestT = T; long boff[DIM]; int bestB[DIM][7];
        int gbestT = T; long gboff[DIM]; int gbestB[DIM][7];
        memcpy(boff, off, sizeof(off)); memcpy(bestB, g_bounds, sizeof(g_bounds));
        memcpy(gboff, boff, sizeof(boff)); memcpy(gbestB, bestB, sizeof(bestB));
        fprintf(stderr, "vj start T=%d\n", T);
        double t0 = now_s(); long it = 0; long last_improve = 0;
        while (now_s() - t0 < budget) {
            it++;
            long o2[DIM]; memcpy(o2, boff, sizeof(o2));
            memcpy(g_bounds, bestB, sizeof(g_bounds));
            int mv = (int)(rnd() % 4ULL);
            if (mv == 0) {                       /* shift one constant */
                int k = (int)(rnd() % DIM);
                o2[k] = (o2[k] + 1 + (long)(rnd() % (uint64_t)(n - 1))) % n;
            } else if (mv == 1) {                /* move one boundary */
                int k = (int)(rnd() % DIM);
                int j = 1 + (int)(rnd() % 5);
                int nv = g_bounds[k][j] + (int)(rnd() % 9) - 4;
                if (nv <= g_bounds[k][j-1] || nv >= g_bounds[k][j+1]) continue;
                g_bounds[k][j] = nv;
            } else if (mv == 2) {                /* move a whole coordinate's partition */
                int k = (int)(rnd() % DIM);
                for (int j = 1; j < 6; j++) {
                    int nv = g_bounds[k][j] + (int)(rnd() % 7) - 3;
                    if (nv > g_bounds[k][j-1] && nv < g_bounds[k][j+1]) g_bounds[k][j] = nv;
                }
            } else {                             /* shift all constants together */
                long d = 1 + (long)(rnd() % (uint64_t)(n - 1));
                for (int k = 0; k < DIM; k++) o2[k] = (o2[k] + d) % n;
            }
            int t2 = evaluate_f(n, v, o2, fv, words);
            if (t2 < 0) continue;
            if (t2 >= bestT) {
                bestT = t2; memcpy(boff, o2, sizeof(o2)); memcpy(bestB, g_bounds, sizeof(g_bounds));
                if (t2 > T) { T = t2; last_improve = it;
                    if (t2 > gbestT) { gbestT = t2; memcpy(gboff, boff, sizeof(boff)); memcpy(gbestB, bestB, sizeof(bestB));
                    fprintf(stderr, "vj GLOBAL BEST T=%d it=%ld\n", gbestT, it); fflush(stderr); }
                    fprintf(stderr, "vj T=%d (off=%ld,%ld,%ld,%ld,%ld) it=%ld\n", T, boff[0],boff[1],boff[2],boff[3],boff[4], it); fflush(stderr); }
            }
            if (it % 2000 == 0) fprintf(stderr, "vj it=%ld best=%d (global %d) %.0fs\n", it, bestT, gbestT, now_s() - t0);
            /* ILS: when stuck, shake the current best hard and continue.
             * The 367 point construction is a sharp local optimum in the
             * joint (constant, partition) space: small moves never escape. */
            if (it - last_improve > 4000) {
                last_improve = it;
                long d = 1 + (long)(rnd() % (uint64_t)(n - 1));
                for (int k = 0; k < DIM; k++) boff[k] = (boff[k] + d) % n;
                for (int k = 0; k < DIM; k++) {
                    long prev = 0;
                    for (int j = 0; j < 7; j++) {
                        long span = (long)n / 7;
                        long b = prev + (long)(rnd() % (uint64_t)(span + 1));
                        if (j == 6) b = n - 1;
                        if (b > n - 1) b = n - 1;
                        if (b < prev) b = prev;
                        bestB[k][j] = (int)b;
                        prev = b;
                    }
                }
                /* Shake the GLOBAL best GENTLY: a small constant shift and a
                 * couple of boundary nudges.  A full randomisation threw the
                 * search into a far-away basin it could not climb back from
                 * within the epoch, so it thrashed instead of progressing. */
                memcpy(boff, gboff, sizeof(boff)); memcpy(bestB, gbestB, sizeof(bestB));
                long d2 = 1 + (long)(rnd() % 8ULL);
                for (int k = 0; k < DIM; k++) boff[k] = (boff[k] + d2) % n;
                int nshake = 1 + (int)(rnd() % 3ULL);
                for (int z = 0; z < nshake; z++) {
                    int k = (int)(rnd() % DIM);
                    int j = 1 + (int)(rnd() % 5);
                    int nv = bestB[k][j] + (int)(rnd() % 7) - 3;
                    if (nv > bestB[k][j-1] && nv < bestB[k][j+1]) bestB[k][j] = nv;
                }
                bestT = evaluate_f(n, v, boff, fv, words);
                if (bestT > gbestT) { gbestT = bestT; memcpy(gboff, boff, sizeof(boff)); memcpy(gbestB, bestB, sizeof(bestB));
                    fprintf(stderr, "vj restart -> T=%d it=%ld\n", gbestT, it); fflush(stderr); }
            }
        }
        printf("VJ n=%ld best=%d (global %d) off=(%ld,%ld,%ld,%ld,%ld)\n", n, bestT, gbestT, gboff[0],gboff[1],gboff[2],gboff[3],gboff[4]);
        fflush(stdout);
        return 0;
    }

    if (!strcmp(mode, "vpolish")) {
        /* vpolish n v0..v4 off0..off4 f : polish an arbitrary base code */
        long n = atol(argv[2]);
        long v[DIM], off[DIM];
        for (int k = 0; k < DIM; k++) v[k] = atol(argv[3 + k]);
        for (int k = 0; k < DIM; k++) off[k] = atol(argv[8 + k]);
        double f = argc > 13 ? atof(argv[13]) : (double)n / 7.0;
        int r = polish_run(n, v, off, f);
        printf("vpolish done: best %d\n", r);
        return 0;
    }

    if (!strcmp(mode, "ls")) {
        /* ls n v0..v4 off0..off4 f <secs> : local search on M (|M| + alpha(G)) */
        long n = atol(argv[2]);
        long v[DIM], off[DIM];
        for (int k = 0; k < DIM; k++) v[k] = atol(argv[3 + k]);
        for (int k = 0; k < DIM; k++) off[k] = atol(argv[8 + k]);
        double f = argc > 13 ? atof(argv[13]) : (double)n / 7.0;
        double budget = argc > 14 ? atof(argv[14]) : 60.0;
        int r = ls_run(n, v, off, f, budget);
        printf("ls done: best %d\n", r);
        return 0;
    }

    if (!strcmp(mode, "vhc")) {
        /* vhc n v0..v4 <secs> <seed> [percoord] : hill-climb (off, f) for one code */
        long n = atol(argv[2]);
        long v[DIM];
        for (int k = 0; k < DIM; k++) v[k] = atol(argv[3 + k]);
        double budget = argc > 8 ? atof(argv[8]) : 60.0;
        uint64_t seed = argc > 9 ? strtoull(argv[9], NULL, 10) : 12345;
        int pc = argc > 10 ? atoi(argv[10]) : 0;
        long soff[DIM]; double sfv[DIM]; int have_start = 0;
        if (argc > 11) {
            for (int k = 0; k < DIM; k++) soff[k] = atol(argv[11 + k]);
            for (int k = 0; k < DIM; k++) sfv[k] = (argc > 16) ? atof(argv[16 + k]) : (double)n / 7.0;
            have_start = 1;
        }
        int r = vhc_run(n, v, budget, seed, pc, have_start ? soff : NULL, have_start ? sfv : NULL);
        printf("vhc done: best %d\n", r);
        return 0;
    }

    if (!strcmp(mode, "sweepfile")) {
        /* reads "n q" lines; tries noff offsets for each */
        const char *file = argv[2];
        uint64_t seed = strtoull(argv[3], NULL, 10);
        long noff = atol(argv[4]);
        int pat = argc > 5 ? atoi(argv[5]) : 0;
        rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        if (!rs) rs = 88172645463325252ULL;
        if (argc > 6 && !strcmp(argv[6], "A")) g_use_rule_b = 0;
        FILE *fp = fopen(file, "r");
        if (!fp) { perror("open list"); return 1; }
        char line[128];
        int words[4096];
        long nev = 0;
        while (fgets(line, sizeof(line), fp)) {
            long n = 0, q = 0;
            if (sscanf(line, "%ld %ld", &n, &q) != 2) continue;
            long v[DIM], pw = 1;
            for (int k = 0; k < DIM; k++) { v[k] = pw % n; pw = (pw * q) % n; }
            for (long t = 0; t < noff; t++) {
                long off[DIM];
                if (pat == 0) {
                    long a = (long)(rnd() % (uint64_t)n), b = (long)(rnd() % (uint64_t)n);
                    off[0] = a; off[1] = b; off[2] = a; off[3] = b; off[4] = a;
                } else if (pat == 1) {
                    for (int j = 0; j < DIM; j++) off[j] = (long)(rnd() % (uint64_t)n);
                } else {
                    off[0] = off[2] = off[4] = (long)(rnd() % (uint64_t)n);
                    off[1] = off[3] = (long)(rnd() % (uint64_t)n);
                }
                double f = (double)n / 7.0;
                int total = evaluate(n, v, off, f, words);
                nev++;
                if (total > best_total) {
                    best_total = total;
                    best_n = n; best_q = q; best_f = f;
                    for (int j = 0; j < DIM; j++) best_off[j] = off[j];
                    memcpy(best_words, words, sizeof(int) * total);
                    best_nwords = total;
                    report("BEST", n, q, v, off, f, total, mcount, gsize);
                    dump_words(getenv("OUT5") ? getenv("OUT5") : "best5.txt", best_words, best_nwords);
                }
            }
        }
        printf("sweepfile done: %ld evaluations, best total %d\n", nev, best_total);
        return 0;
    }

    printf("unknown mode\n");
    return 1;
}
