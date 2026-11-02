/* search3.c -- LABS n=100: multi-operator ILS with a configurable objective.
 *
 * E = sum_{k=1}^{n-1} C_k^2 , C_k = sum_i s_i s_{i+k}
 *
 * The search minimises a generalisable objective  Q = sum_k f(C_k)  with
 *   f(c) = c^2                                  (mode 0: true energy)
 *   f(c) = c^2 + lam*max(0,|c|-c0)^2            (mode 1: smoothed landscape)
 *   f(c) = c^2 + lam2*c^4                       (mode 2: quartic emphasis)
 *   f(c) = c^2 + mu*|c|                         (mode 3: L1 emphasis)
 * The true energy E is tracked separately and is what gets reported.
 *
 * Operators : tabu / sa_best (Metropolis on best move) / ta (threshold acc.)
 * Polish    : greedy 1-flip descent, LNS (exact block re-optimisation, Gray code),
 *             exhaustive 2-flip (swap) polish
 * Modes     : ils (iterated local search), mem (memetic algorithm with crossover)
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>

#define N 100
#define MAXM 64
#define MAXPOP 32

/* ------------------------------------------------------------------ RNG */
static uint64_t rngs[4];
static inline uint64_t rotl64(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
static inline uint64_t rnd64(void) {
    uint64_t r = rotl64(rngs[1] * 5ULL, 7) * 9ULL;
    uint64_t t = rngs[1] << 17;
    rngs[2] ^= rngs[0]; rngs[3] ^= rngs[1]; rngs[1] ^= rngs[2];
    rngs[0] ^= rngs[3]; rngs[2] ^= t;
    rngs[3] = rotl64(rngs[3], 45);
    return r;
}
static inline int rndint(int n) { return (int)(rnd64() % (uint64_t)n); }
static inline double rnd01(void) { return (double)(rnd64() >> 11) * (1.0 / 9007199254740992.0); }
static void seed_rng(uint64_t seed) {
    for (int i = 0; i < 4; i++) {
        seed += 0x9E3779B97F4A7C15ULL;
        uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        rngs[i] = z ^ (z >> 31);
    }
    for (int i = 0; i < 20; i++) rnd64();
}

/* --------------------------------------------------------- objective */
static int obj_mode = 0;
static double obj_lam = 0.0, obj_c0 = 2.0, obj_lam2 = 0.0, obj_mu = 0.0;

static inline long long fval(int c) {
    long long q = (long long)c * c;
    switch (obj_mode) {
        case 1: {
            long long a = c < 0 ? -c : c;
            long long ex = a - (long long)obj_c0;
            if (ex < 0) ex = 0;
            return q + (long long)obj_lam * ex * ex;
        }
        case 2: {
            long long q2 = q * q / 1000;
            return q + (long long)obj_lam2 * q2;
        }
        case 3: {
            long long a = c < 0 ? -c : c;
            return q + (long long)obj_mu * a;
        }
    }
    return q;
}

/* -------------------------------------------------------------- state */
static int s[N];
static int Cv[N + 1];
static int Ecur;            /* true energy   */
static long long Qcur;       /* objective     */

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + 1e-9 * (double)ts.tv_nsec;
}

static void recompute_all(void) {
    for (int k = 1; k < N; k++) {
        int c = 0;
        for (int i = 0, m = N - k; i < m; i++) c += s[i] * s[i + k];
        Cv[k] = c;
    }
    int e = 0;
    long long q = 0;
    for (int k = 1; k < N; k++) { e += Cv[k] * Cv[k]; q += fval(Cv[k]); }
    Ecur = e;
    Qcur = q;
}

/* objective delta if s[j] is flipped (no mutation) */
static inline long long obj_delta(const int *ss, const int *cc, int j) {
    if (obj_mode == 0) {                       /* fast path: pure energy */
        const int sj = ss[j];
        const int lo = (j < N - 1 - j) ? j : (N - 1 - j);
        const int hi = (j < N - 1 - j) ? (N - 1 - j) : j;
        int d = 0, k;
        for (k = 1; k <= lo; k++) {
            int dc = -2 * sj * (ss[j - k] + ss[j + k]);
            d += 2 * cc[k] * dc + dc * dc;
        }
        if (hi > lo) {
            if (j < N - 1 - j) {
                for (; k <= hi; k++) { int dc = -2 * sj * ss[j + k]; d += 2 * cc[k] * dc + dc * dc; }
            } else {
                for (; k <= hi; k++) { int dc = -2 * sj * ss[j - k]; d += 2 * cc[k] * dc + dc * dc; }
            }
        }
        return d;
    }
    const int sj = ss[j];
    const int lo = (j < N - 1 - j) ? j : (N - 1 - j);
    const int hi = (j < N - 1 - j) ? (N - 1 - j) : j;
    long long d = 0;
    int k;
    for (k = 1; k <= lo; k++) {
        int dc = -2 * sj * (ss[j - k] + ss[j + k]);
        d += fval(cc[k] + dc) - fval(cc[k]);
    }
    if (hi > lo) {
        if (j < N - 1 - j) {
            for (; k <= hi; k++) { int dc = -2 * sj * ss[j + k]; d += fval(cc[k] + dc) - fval(cc[k]); }
        } else {
            for (; k <= hi; k++) { int dc = -2 * sj * ss[j - k]; d += fval(cc[k] + dc) - fval(cc[k]); }
        }
    }
    return d;
}

static inline void apply_flip(int j) {
    if (obj_mode == 0) {                       /* fast path: pure energy */
        const int sj = s[j];
        const int lo = (j < N - 1 - j) ? j : (N - 1 - j);
        const int hi = (j < N - 1 - j) ? (N - 1 - j) : j;
        int de = 0, k;
        for (k = 1; k <= lo; k++) {
            int dc = -2 * sj * (s[j - k] + s[j + k]);
            int old = Cv[k]; Cv[k] = old + dc;
            de += 2 * old * dc + dc * dc;
        }
        if (hi > lo) {
            if (j < N - 1 - j) {
                for (; k <= hi; k++) { int dc = -2 * sj * s[j + k]; int old = Cv[k]; Cv[k] = old + dc; de += 2 * old * dc + dc * dc; }
            } else {
                for (; k <= hi; k++) { int dc = -2 * sj * s[j - k]; int old = Cv[k]; Cv[k] = old + dc; de += 2 * old * dc + dc * dc; }
            }
        }
        s[j] = -sj;
        Ecur += de;
        Qcur = 0;
        for (int q = 1; q < N; q++) Qcur += (long long)Cv[q] * Cv[q];
        return;
    }
    const int sj = s[j];
    const int lo = (j < N - 1 - j) ? j : (N - 1 - j);
    const int hi = (j < N - 1 - j) ? (N - 1 - j) : j;
    int de = 0;
    long long dq = 0;
    int k;
    for (k = 1; k <= lo; k++) {
        int dc = -2 * sj * (s[j - k] + s[j + k]);
        int old = Cv[k];
        Cv[k] = old + dc;
        de += 2 * old * dc + dc * dc;
        dq += fval(Cv[k]) - fval(old);
    }
    if (hi > lo) {
        if (j < N - 1 - j) {
            for (; k <= hi; k++) {
                int dc = -2 * sj * s[j + k];
                int old = Cv[k]; Cv[k] = old + dc;
                de += 2 * old * dc + dc * dc; dq += fval(Cv[k]) - fval(old);
            }
        } else {
            for (; k <= hi; k++) {
                int dc = -2 * sj * s[j - k];
                int old = Cv[k]; Cv[k] = old + dc;
                de += 2 * old * dc + dc * dc; dq += fval(Cv[k]) - fval(old);
            }
        }
    }
    s[j] = -sj;
    Ecur += de;
    Qcur += dq;
}

/* ---------------------------------------------------------- I/O */
static int load_seq_buf(const char *path, char *out) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    static char buf[65536];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = 0;
    int cnt = 0, st = -1;
    for (size_t i = 0; i < n; i++) {
        if (buf[i] == '+' || buf[i] == '-') {
            if (st < 0) st = (int)i;
            if (++cnt == N) { memcpy(out, buf + st, N); out[N] = 0; return 1; }
        } else { cnt = 0; st = -1; }
    }
    return 0;
}
static void save_seq_file(const char *path, const char *seq) {
    char buf[128];
    memcpy(buf, seq, N); buf[N] = 0;
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "%s\n", buf);
    fclose(f);
}
static void seq_to_str(const int *src, char *dst) {
    for (int i = 0; i < N; i++) dst[i] = (src[i] > 0) ? '+' : '-';
    dst[N] = 0;
}
static void str_to_seq(const char *src, int *dst) {
    for (int i = 0; i < N; i++) dst[i] = (src[i] == '+') ? 1 : -1;
}

/* --------------------------------------------------- greedy descent */
static void greedy_descent(void) {
    for (;;) {
        int bestj = -1;
        long long bestd = 0;
        for (int j = 0; j < N; j++) {
            long long d = obj_delta(s, Cv, j);
            if (bestj < 0 || d < bestd) { bestd = d; bestj = j; }
        }
        if (bestj < 0 || bestd >= 0) break;
        apply_flip(bestj);
    }
}

/* ------------------------------------------------------------ LNS */
static int ts[N], tc[N + 1];
static long long tq;

static int lns_set(const int *pos, int m) {
    memcpy(ts, s, sizeof(int) * N);
    memcpy(tc, Cv, sizeof(int) * (N + 1));
    tq = Qcur;
    long long bestQ = tq;
    unsigned bestmask = 0, mask = 0;
    unsigned total = 1u << m;
    for (unsigned step = 1; step < total; step++) {
        unsigned diff = step & (~step + 1u);
        int b = __builtin_ctz(diff);
        int j = pos[b];
        const int sj = ts[j];
        const int lo = (j < N - 1 - j) ? j : (N - 1 - j);
        const int hi = (j < N - 1 - j) ? (N - 1 - j) : j;
        long long dq = 0;
        int k;
        for (k = 1; k <= lo; k++) {
            int dc = -2 * sj * (ts[j - k] + ts[j + k]);
            int old = tc[k]; tc[k] = old + dc;
            dq += fval(tc[k]) - fval(old);
        }
        if (hi > lo) {
            if (j < N - 1 - j) {
                for (; k <= hi; k++) { int dc = -2 * sj * ts[j + k]; int old = tc[k]; tc[k] = old + dc; dq += fval(tc[k]) - fval(old); }
            } else {
                for (; k <= hi; k++) { int dc = -2 * sj * ts[j - k]; int old = tc[k]; tc[k] = old + dc; dq += fval(tc[k]) - fval(old); }
            }
        }
        ts[j] = -sj;
        tq += dq;
        mask ^= diff;
        if (tq < bestQ) { bestQ = tq; bestmask = mask; }
    }
    if (bestQ < Qcur) {
        for (int i = 0; i < m; i++)
            if (bestmask & (1u << i)) s[pos[i]] = -s[pos[i]];
        recompute_all();
        return 1;
    }
    return 0;
}

static int lns_slide(int m, int rounds) {
    int improved = 0;
    int pos[MAXM];
    for (int r = 0; r < rounds; r++) {
        int any = 0;
        for (int a = 0; a + m <= N; a++) {
            for (int i = 0; i < m; i++) pos[i] = a + i;
            if (lns_set(pos, m)) { any = 1; improved = 1; }
        }
        if (!any) break;
    }
    return improved;
}
static int lns_cyclic(int m, int rounds) {
    int improved = 0;
    int pos[MAXM];
    for (int r = 0; r < rounds; r++) {
        int any = 0;
        for (int a = 0; a < N; a++) {
            for (int i = 0; i < m; i++) pos[i] = (a + i) % N;
            if (lns_set(pos, m)) { any = 1; improved = 1; }
        }
        if (!any) break;
    }
    return improved;
}
static int lns_random(int m, int rounds, int nsets) {
    int improved = 0;
    int pos[MAXM];
    for (int r = 0; r < rounds; r++) {
        int any = 0;
        for (int t = 0; t < nsets; t++) {
            for (int i = 0; i < m; i++) pos[i] = rndint(N);
            if (lns_set(pos, m)) { any = 1; improved = 1; }
        }
        if (!any) break;
    }
    return improved;
}

/* --------------------------------------------------------- swap polish */
static int dcp[N][N];
static int swap_polish(void) {
    for (int j = 0; j < N; j++) {
        const int sj = s[j];
        const int lo = (j < N - 1 - j) ? j : (N - 1 - j);
        const int hi = (j < N - 1 - j) ? (N - 1 - j) : j;
        for (int k = 1; k < N; k++) {
            int b = 0;
            if (k <= lo) b = s[j - k] + s[j + k];
            else if (k <= hi) b = (j < N - 1 - j) ? s[j + k] : s[j - k];
            dcp[j][k] = -2 * sj * b;
        }
    }
    long long Ab[N], Bb[N];
    for (int j = 0; j < N; j++) {
        long long a = 0, b = 0;
        for (int k = 1; k < N; k++) { a += 2LL * Cv[k] * dcp[j][k]; b += (long long)dcp[j][k] * dcp[j][k]; }
        Ab[j] = a; Bb[j] = b;
    }
    int bestpair = -1;
    long long bestdE = 0;
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            int k0 = j - i;
            int corr = 4 * s[i] * s[j];
            long long cross = 0;
            int km = (i > N - 1 - i ? i : N - 1 - i);
            int km2 = (j > N - 1 - j ? j : N - 1 - j);
            if (km2 < km) km = km2;
            for (int k = 1; k <= km; k++) cross += (long long)dcp[i][k] * dcp[j][k];
            long long dE = Ab[i] + Ab[j] + Bb[i] + Bb[j] + 2 * cross + 2LL * Cv[k0] * corr
                         + 2LL * corr * (dcp[i][k0] + dcp[j][k0]) + (long long)corr * corr;
            if (bestpair < 0 || dE < bestdE) { bestdE = dE; bestpair = i * N + j; }
        }
    }
    if (bestpair >= 0 && bestdE < 0) {
        apply_flip(bestpair / N);
        apply_flip(bestpair % N);
        return 1;
    }
    return 0;
}

/* --------------------------------------------------------- tabu search */
static int gbestE = 1 << 30;
static char gbest[128];
static long long gbestQ = (long long)1 << 62;   /* best objective ever seen */
#define MAXARCH 24
static char arch[MAXARCH][N + 1];                /* archive of elite solutions */
static int archE[MAXARCH];
static int archN = 0;

static int tabu_chunk(long iters, int tmin, int tmax, int use_aspiration) {
    int tabu_until[N];
    for (int i = 0; i < N; i++) tabu_until[i] = -1;
    long long bestQ = Qcur;
    char run_best[128];
    seq_to_str(s, run_best);
    for (long it = 0; it < iters; it++) {
        int bestj = -1, any = 0;
        long long bestd = 0;
        long long asp = use_aspiration ? gbestQ : bestQ;
        for (int j = 0; j < N; j++) {
            long long d = obj_delta(s, Cv, j);
            if (it >= tabu_until[j] || (Qcur + d < asp)) {
                if (!any || d < bestd) { bestd = d; bestj = j; any = 1; }
            }
        }
        if (!any) {
            int oldestT = 1 << 30, oj = 0;
            for (int j = 0; j < N; j++) if (tabu_until[j] < oldestT) { oldestT = tabu_until[j]; oj = j; }
            bestj = oj;
        }
        apply_flip(bestj);
        tabu_until[bestj] = it + tmin + rndint(tmax - tmin + 1);
        if (Qcur < bestQ) {
            bestQ = Qcur;
            seq_to_str(s, run_best);
        }
    }
    str_to_seq(run_best, s);
    recompute_all();
    return 0;
}

/* ------------------------------------------------- best-improvement SA */
static double exptab[MAXM + 1];
static inline void refresh_exptab(double T) {
    for (int m = 1; m <= MAXM; m++) exptab[m] = exp(-4.0 * (double)m / T);
}
static int sa_best(double T0, double T1, long steps) {
    double lr = log(T1 / T0);
    long long bestQ = Qcur;
    char run_best[128];
    seq_to_str(s, run_best);
    double T = T0;
    long refresh = 0;
    for (long it = 0; it < steps; it++) {
        if (--refresh <= 0) { refresh = 128; refresh_exptab(T); }
        int bestj = -1;
        long long bestd = 0;
        for (int j = 0; j < N; j++) {
            long long d = obj_delta(s, Cv, j);
            if (bestj < 0 || d < bestd) { bestd = d; bestj = j; }
        }
        if (bestd <= 0) apply_flip(bestj);
        else {
            double dd = (double)bestd;
            if (dd < 0) dd = -dd;
            double p = exp(-dd / T);
            if (rnd01() < p) apply_flip(bestj);
        }
        if (Qcur < bestQ) { bestQ = Qcur; seq_to_str(s, run_best); }
        T = T0 * exp(lr * (double)(it + 1) / (double)steps);
    }
    str_to_seq(run_best, s);
    recompute_all();
    return 0;
}

/* ------------------------------------------- threshold accepting (best) */
static int ta_best(double T0, double T1, long steps) {
    long long bestQ = Qcur;
    char run_best[128];
    seq_to_str(s, run_best);
    for (long it = 0; it < steps; it++) {
        double frac = (double)(it + 1) / (double)steps;
        double thr = T0 + (T1 - T0) * frac;
        int bestj = -1;
        long long bestd = 0;
        for (int j = 0; j < N; j++) {
            long long d = obj_delta(s, Cv, j);
            if (bestj < 0 || d < bestd) { bestd = d; bestj = j; }
        }
        if ((double)bestd <= thr) apply_flip(bestj);
        if (Qcur < bestQ) { bestQ = Qcur; seq_to_str(s, run_best); }
    }
    str_to_seq(run_best, s);
    recompute_all();
    return 0;
}


static void report_if_better(void);

/* path relinking: walk from the incumbent towards a target sequence, one bit at
   a time, polishing each step; keeps the best visited state. */
static void path_relink(const char *target) {
    int tgt[N];
    str_to_seq(target, tgt);
    str_to_seq(gbest, s);
    recompute_all();
    int diff[MAXM], nd = 0;
    for (int i = 0; i < N; i++) if (s[i] != tgt[i]) diff[nd++] = i;
    /* shuffle */
    for (int i = nd - 1; i > 0; i--) { int j = rndint(i + 1); int t = diff[i]; diff[i] = diff[j]; diff[j] = t; }
    for (int i = 0; i < nd; i++) {
        apply_flip(diff[i]);
        greedy_descent();
        if (Ecur < gbestE) report_if_better();
    }
    /* a few "away" steps for extra exploration */
    int away = 3 + rndint(6);
    for (int i = 0; i < away; i++) {
        apply_flip(rndint(N));
        greedy_descent();
        if (Ecur < gbestE) report_if_better();
    }
}

/* ----------------------------------------------------------- global */
static const char *g_out = NULL;
static FILE *g_log = NULL;
static double t_start;

static void logf_(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap; va_start(ap, fmt); vfprintf(g_log, fmt, ap); va_end(ap);
    fflush(g_log);
}
static void arch_insert(void) {
    /* keep the archive sorted by E, at most MAXARCH entries */
    if (archN < MAXARCH) {
        int i = archN++;
        while (i > 0 && archE[i - 1] > Ecur) { archE[i] = archE[i - 1]; memcpy(arch[i], arch[i - 1], N + 1); i--; }
        archE[i] = Ecur; memcpy(arch[i], gbest, N + 1);
    } else if (Ecur < archE[MAXARCH - 1]) {
        int i = MAXARCH - 1;
        while (i > 0 && archE[i - 1] > Ecur) { archE[i] = archE[i - 1]; memcpy(arch[i], arch[i - 1], N + 1); i--; }
        archE[i] = Ecur; memcpy(arch[i], gbest, N + 1);
    }
}
static void report_if_better(void) {
    if (Qcur < gbestQ) gbestQ = Qcur;
    if (Ecur < gbestE) {
        gbestE = Ecur;
        seq_to_str(s, gbest);
        if (g_out) {
            save_seq_file(g_out, gbest);
            char pb[512]; snprintf(pb, sizeof(pb), "%s.best", g_out);
            save_seq_file(pb, gbest);
        }
        logf_("[%.1f] E=%d\n", now_sec() - t_start, Ecur);
        arch_insert();
    }
}
static void load_state(const char *seq) {
    str_to_seq(seq, s);
    recompute_all();
}

int main(int argc, char **argv) {
    const char *seed_file = NULL, *warm_file = NULL, *out_path = NULL, *mode = "ils";
    const char *op = "tabu";
    uint64_t seed = 12345;
    double limit = 3600.0, T0 = 0.6, T1 = 0.05, p_smooth = 0.0, p_strong = 0.15;
    long steps = 2000000, chunk = 200000;
    int tmin = 5, tmax = 15, lns_m = 10, lns_rounds = 3, verbose = 0, use_asp = 1;
    int lns_cyc = 0, lns_rand = 0, swap_on = 0, pop_size = 16, xover_m = 14;
    double mem_mut = 0.1, p_arch = 0.0, p_prl = 0.0; int esc_inc = 0, mem_replace = 0;
    int xover_lns = 0, xover_cap = 16;
    double restart_p = 0.0, pert = 0.0; int patience = 0, pert_mode = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seed-file") && i + 1 < argc) seed_file = argv[++i];
        else if (!strcmp(argv[i], "--warm") && i + 1 < argc) warm_file = argv[++i];
        else if (!strcmp(argv[i], "--out") && i + 1 < argc) out_path = argv[++i];
        else if (!strcmp(argv[i], "--mode") && i + 1 < argc) mode = argv[++i];
        else if (!strcmp(argv[i], "--op") && i + 1 < argc) op = argv[++i];
        else if (!strcmp(argv[i], "--rng") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--t0") && i + 1 < argc) T0 = atof(argv[++i]);
        else if (!strcmp(argv[i], "--t1") && i + 1 < argc) T1 = atof(argv[++i]);
        else if (!strcmp(argv[i], "--steps") && i + 1 < argc) steps = atol(argv[++i]);
        else if (!strcmp(argv[i], "--chunk") && i + 1 < argc) chunk = atol(argv[++i]);
        else if (!strcmp(argv[i], "--tmin") && i + 1 < argc) tmin = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--tmax") && i + 1 < argc) tmax = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--lns-m") && i + 1 < argc) lns_m = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--lns-rounds") && i + 1 < argc) lns_rounds = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--lns-cyc") && i + 1 < argc) lns_cyc = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--lns-rand") && i + 1 < argc) lns_rand = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--swap")) swap_on = 1;
        else if (!strcmp(argv[i], "--time") && i + 1 < argc) limit = atof(argv[++i]);
        else if (!strcmp(argv[i], "--p-strong") && i + 1 < argc) p_strong = atof(argv[++i]);
        else if (!strcmp(argv[i], "--p-smooth") && i + 1 < argc) p_smooth = atof(argv[++i]);
        else if (!strcmp(argv[i], "--obj") && i + 1 < argc) obj_mode = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--obj-lam") && i + 1 < argc) obj_lam = atof(argv[++i]);
        else if (!strcmp(argv[i], "--obj-c0") && i + 1 < argc) obj_c0 = atof(argv[++i]);
        else if (!strcmp(argv[i], "--obj-lam2") && i + 1 < argc) obj_lam2 = atof(argv[++i]);
        else if (!strcmp(argv[i], "--obj-mu") && i + 1 < argc) obj_mu = atof(argv[++i]);
        else if (!strcmp(argv[i], "--pop") && i + 1 < argc) pop_size = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--xover-m") && i + 1 < argc) xover_m = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mem-mut") && i + 1 < argc) mem_mut = atof(argv[++i]);
        else if (!strcmp(argv[i], "--p-arch") && i + 1 < argc) p_arch = atof(argv[++i]);
        else if (!strcmp(argv[i], "--esc-inc") && i + 1 < argc) esc_inc = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mem-replace") && i + 1 < argc) mem_replace = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--xover-lns") && i + 1 < argc) xover_lns = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--xover-cap") && i + 1 < argc) xover_cap = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--p-prl") && i + 1 < argc) p_prl = atof(argv[++i]);
        else if (!strcmp(argv[i], "--restart-p") && i + 1 < argc) restart_p = atof(argv[++i]);
        else if (!strcmp(argv[i], "--pert") && i + 1 < argc) pert = atof(argv[++i]);
        else if (!strcmp(argv[i], "--patience") && i + 1 < argc) patience = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--pert-block")) pert_mode = 1;
        else if (!strcmp(argv[i], "--no-aspiration")) use_asp = 0;
        else if (!strcmp(argv[i], "--verbose")) verbose = 1;
        else { fprintf(stderr, "unknown arg %s\n", argv[i]); return 2; }
    }

    seed_rng(seed);
    t_start = now_sec();
    g_out = out_path;

    char seq[N + 1];
    if (warm_file || seed_file) {
        const char *f = warm_file ? warm_file : seed_file;
        if (!load_seq_buf(f, seq)) { fprintf(stderr, "cannot read %s\n", f); return 2; }
    } else {
        for (int i = 0; i < N; i++) seq[i] = (rndint(2)) ? '+' : '-';
        seq[N] = 0;
    }
    load_state(seq);

    gbestE = Ecur;
    gbestQ = Qcur;
    seq_to_str(s, gbest);

    char logpath[512];
    snprintf(logpath, sizeof(logpath), "%s.log", out_path ? out_path : "search3");
    g_log = fopen(logpath, "w");
    logf_("start E=%d rng=%llu mode=%s op=%s obj=%d lam=%g c0=%g tmin=%d tmax=%d chunk=%ld lns_m=%d\n",
          Ecur, (unsigned long long)seed, mode, op, obj_mode, obj_lam, obj_c0, tmin, tmax, chunk, lns_m);
    if (g_out) { save_seq_file(g_out, gbest); char pb[512]; snprintf(pb, sizeof(pb), "%s.best", g_out); save_seq_file(pb, gbest); }

    if (!strcmp(mode, "eval")) { printf("E=%d\n", Ecur); return 0; }
    if (!strcmp(mode, "selftest")) {
        int bad = 0;
        for (int trial = 0; trial < 300; trial++) {
            recompute_all();
            int j = rndint(N);
            apply_flip(j);
            int e1 = Ecur; long long q1 = Qcur;
            recompute_all();
            if (e1 != Ecur || q1 != Qcur) { printf("MISMATCH j=%d %d vs %d\n", j, e1, Ecur); bad++; }
        }
        printf("selftest mismatches=%d E=%d mod4=%d\n", bad, Ecur, Ecur % 4);
        return bad ? 1 : 0;
    }

    /* operator dispatcher */
    #define RUN_OP(CH) do { \
        if (!strcmp(op, "tabu")) tabu_chunk(CH, tmin, tmax, use_asp); \
        else if (!strcmp(op, "sabest")) sa_best(T0, T1, CH); \
        else if (!strcmp(op, "tabest")) ta_best(T0, T1, CH); \
    } while (0)

    if (!strcmp(mode, "single")) {
        RUN_OP(steps);
        greedy_descent();
        if (swap_on) { int q = 0; while (swap_polish() && q < 20) q++; }
        if (lns_m > 0) lns_slide(lns_m, lns_rounds);
        report_if_better();
    } else if (!strcmp(mode, "mem")) {
        /* memetic algorithm: population + 1-point crossover + seam repair */
        static int pop[MAXPOP][N];
        static int popE[MAXPOP];
        int P = pop_size;
        for (int i = 0; i < P; i++) {
            if (i % 2 == 0) {
                /* half of the population starts from the incumbent (perturbed) */
                memcpy(s, gbest, N + 1);
                str_to_seq(gbest, s);
                recompute_all();
                int nf = 2 + rndint(6);
                for (int q = 0; q < nf; q++) apply_flip(rndint(N));
            } else {
                for (int k = 0; k < N; k++) pop[i][k] = (rndint(2)) ? 1 : -1;
                memcpy(s, pop[i], sizeof(int) * N);
                recompute_all();
            }
            RUN_OP(chunk / 4);
            greedy_descent();
            if (lns_m > 0) lns_slide(lns_m, 1);
            memcpy(pop[i], s, sizeof(int) * N);
            popE[i] = Ecur;
            report_if_better();
        }
        while (now_sec() - t_start < limit) {
            /* pick two parents (tournament) */
            int i1 = rndint(P), i2 = rndint(P);
            for (int t = 0; t < 3; t++) { int c = rndint(P); if (popE[c] < popE[i1]) i1 = c; c = rndint(P); if (popE[c] < popE[i2]) i2 = c; }
            int k = 1 + rndint(N - 1);
            if (rnd01() < mem_mut) {
                /* mutation of a single parent */
                memcpy(s, pop[i1], sizeof(int) * N);
                recompute_all();
                int nf = 1 + rndint(4);
                for (int q = 0; q < nf; q++) apply_flip(rndint(N));
            } else if (xover_lns && xover_cap > 0) {
                /* exact recombination: if the parents differ in few positions,
                   optimise exactly over those positions (LNS) starting from A */
                int diffpos[MAXM], nd = 0;
                for (int q = 0; q < N; q++) if (pop[i1][q] != pop[i2][q]) diffpos[nd++] = q;
                if (nd >= 2 && nd <= xover_cap) {
                    for (int q = 0; q < N; q++) s[q] = pop[i1][q];
                    recompute_all();
                    lns_set(diffpos, nd);
                } else {
                    for (int q = 0; q < N; q++) s[q] = (q < k) ? pop[i1][q] : pop[i2][q];
                    recompute_all();
                }
            } else {
                for (int q = 0; q < N; q++) s[q] = (q < k) ? pop[i1][q] : pop[i2][q];
                recompute_all();
            }
            /* seam repair: optimise a window around the crossover point */
            int pos[MAXM];
            int mm = xover_m < N ? xover_m : N;
            int st = k - mm / 2; if (st < 0) st = 0; if (st + mm > N) st = N - mm;
            for (int q = 0; q < mm; q++) pos[q] = st + q;
            lns_set(pos, mm);
            RUN_OP(chunk);
            greedy_descent();
            if (swap_on) { int q2 = 0; while (swap_polish() && q2 < 20) q2++; }
            if (lns_m > 0) lns_slide(lns_m, lns_rounds);
            /* replacement: worst individual, or a random member of the worse half
               (random replacement keeps the population diverse) */
            int worst = 0;
            for (int q = 0; q < P; q++) if (popE[q] > popE[worst]) worst = q;
            int victim = worst;
            if (mem_replace == 1) {
                /* pick a random individual among the worse half */
                int half = P / 2, thr = popE[worst];
                for (int q = 0; q < P; q++) if (popE[q] > thr) thr = popE[q];
                int cand[MAXPOP], nc = 0;
                for (int q = 0; q < P; q++) if (popE[q] >= popE[worst]) cand[nc++] = q;
                (void)half; (void)thr;
                if (nc > 0) victim = cand[rndint(nc)];
            }
            int dup = 0;
            for (int q = 0; q < P; q++) if (memcmp(pop[q], s, sizeof(int) * N) == 0) { dup = 1; break; }
            if (!dup && Ecur < popE[victim]) { memcpy(pop[victim], s, sizeof(int) * N); popE[victim] = Ecur; }
            report_if_better();
        }
    } else {
        /* iterated local search */
        long round = 0;
        int since_improve = 0, esc_level = 0;
        while (now_sec() - t_start < limit) {
            round++;
            /* occasional objective smoothing round */
            int saved_mode = obj_mode;
            if (p_smooth > 0 && rnd01() < p_smooth) {
                obj_mode = 1;
                recompute_all();
            }
            /* diversification: restart from a random sequence */
            int do_restart = 0;
            if (restart_p > 0 && rnd01() < restart_p) do_restart = 1;
            if (patience > 0 && since_improve >= patience) do_restart = 1;
            if (do_restart) {
                for (int i = 0; i < N; i++) s[i] = (rndint(2)) ? 1 : -1;
                recompute_all();
            } else if (archN > 1 && p_arch > 0 && rnd01() < p_arch) {
                load_state(arch[rndint(archN)]);     /* diversify over the archive */
            } else {
                load_state(gbest);
            }
            /* path-relinking move towards an archive member */
            if (archN > 1 && p_prl > 0 && rnd01() < p_prl) {
                path_relink(arch[rndint(archN)]);
                load_state(gbest);
                goto after_perturb;
            }
after_perturb:
            double r = rnd01();
            int nflip;
            if (pert > 0) nflip = (int)pert + rndint(3);
            else if (r < p_strong) nflip = 4 + rndint(10);
            else if (r < p_strong + 0.25) nflip = 2 + rndint(3);
            else nflip = 1;
            /* escalation: after `patience` fruitless rounds, perturb harder */
            if (esc_inc > 0 && since_improve >= patience && patience > 0) {
                esc_level += esc_inc;
                since_improve = 0;
                if (esc_level > 60) { esc_level = 0; do_restart = 1; }
                else nflip += 2 * esc_level;
                if (nflip > 60) nflip = 60;
            }
            if (pert_mode == 1) {
                /* flip a contiguous (cyclic) block of nflip bits */
                int st = rndint(N);
                for (int i = 0; i < nflip; i++) apply_flip((st + i) % N);
            } else {
                for (int i = 0; i < nflip; i++) apply_flip(rndint(N));
            }
            RUN_OP(chunk);
            greedy_descent();
            if (swap_on) { int q = 0; while (swap_polish() && q < 20) q++; }
            if (lns_m > 0) {
                lns_slide(lns_m, lns_rounds);
                if (lns_cyc) lns_cyclic(lns_m, 1);
                if (lns_rand) lns_random(lns_m, 1, lns_rand);
            }
            if (obj_mode != saved_mode) { obj_mode = saved_mode; recompute_all(); }
            int before2 = gbestE;
            report_if_better();
            if (gbestE < before2) since_improve = 0; else since_improve++;
            if (verbose && (round % 50 == 0)) logf_("round %ld E=%d best=%d\n", round, Ecur, gbestE);
        }
        logf_("rounds=%ld best=%d\n", round, gbestE);
    }

    printf("BEST E=%d\n", gbestE);
    for (int i = 0; i < N; i++) putchar(gbest[i]);
    putchar('\n');
    if (g_log) fclose(g_log);
    return 0;
}
