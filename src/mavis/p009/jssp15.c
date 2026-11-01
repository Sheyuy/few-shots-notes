/* jssp.c -- tabu search for job-shop instance ta41 (30 jobs, 20 machines).
 *
 * Solution = the 20 machine orders (each a permutation of the 30 jobs) = exactly
 * the certificate format of p009.
 *
 * Neighbourhoods (all derived from the critical path of the disjunctive graph):
 *   N1  van Laarhoven/Aarts/Lenstra/Ulder'92 : swap any adjacent pair of
 *       *critical* operations on the same machine.
 *   N5  Nowicki/Smutnicki'96                : swap the first two / last two
 *       operations of every critical block (critical block = maximal run of
 *       consecutive operations of one machine that are all critical).
 *   N6  Balas/Vazacopoulos'88                : move an operation of a critical
 *       block to the beginning or the end of that block.
 *   N8  Xie/Li/Gao/Gui'21                    : move an *inner* operation of a
 *       critical block outside the block (just before its first or just after
 *       its last operation on that machine).
 *
 * Build: gcc -O3 -march=native -o jssp jssp.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>
#include <signal.h>

#include "instance.h"

#define NJ 30
#define NM 20
#define NOPS 600
#define EPS 0

/* ------------------------------------------------------------------ data */
static int dur[NOPS], jpred[NOPS], jsucc[NOPS], jobof[NOPS];
static int mseq[NM][NJ];          /* op ids per machine, in order          */
static int mprev_[NOPS], mnext_[NOPS];

static int est_[NOPS], topo_[NOPS], tail_[NOPS];
static int n_est[NOPS], n_topo[NOPS];
static int indeg_[NOPS], stack_[NOPS];
static unsigned char crit[NOPS];
static int pos_[NOPS];            /* position of an op inside topo_          */
static unsigned char done_[NOPS];
static int red_[NOPS];            /* scratch: topological order without u    */

/* ------------------------------------------------------------------- rng */
static uint64_t rng_s = 0x9E3779B97F4A7C15ULL;
static inline uint64_t rnd64(void) {
    uint64_t x = rng_s;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    rng_s = x;
    return x;
}
static inline uint32_t rnd(uint32_t n) { return (uint32_t)(rnd64() % n); }

/* ---------------------------------------------------------------- eval   */
/* Longest path in the disjunctive graph.  Returns the makespan, or cutoff+1
 * as soon as it is certain that the makespan exceeds cutoff (exact pruning:
 * every est[u]+dur[u] seen is the length of a real path, hence a lower bound
 * of the makespan).  -1 if the graph has a cycle.                        */
static int last_aborted = 0;
static int burst_iters = 0;
static int eval_kahn(int *est, int *topo, int cutoff) {
    last_aborted = 0;
    int sp = 0, cnt = 0, end = 0;
    for (int o = 0; o < NOPS; o++) {
        int d = (jpred[o] >= 0) + (mprev_[o] >= 0);
        indeg_[o] = d;
        est[o] = 0;
        if (d == 0) stack_[sp++] = o;
    }
    while (sp) {
        int u = stack_[--sp];
        topo[cnt++] = u;
        int fin = est[u] + dur[u];
        if (fin > end) {
            end = fin;
            if (end > cutoff) { last_aborted = 1; return cutoff + 1; }
        }
        int v = jsucc[u];
        if (v >= 0) {
            if (fin > est[v]) est[v] = fin;
            if (--indeg_[v] == 0) stack_[sp++] = v;
        }
        v = mnext_[u];
        if (v >= 0) {
            if (fin > est[v]) est[v] = fin;
            if (--indeg_[v] == 0) stack_[sp++] = v;
        }
    }
    if (cnt != NOPS) return -1;
    return end;
}

static void calc_tails(int *topo, int cnt) {
    for (int i = cnt - 1; i >= 0; i--) {
        int u = topo[i];
        int t = 0, v, s;
        v = jsucc[u];
        if (v >= 0) { s = dur[v] + tail_[v]; if (s > t) t = s; }
        v = mnext_[u];
        if (v >= 0) { s = dur[v] + tail_[v]; if (s > t) t = s; }
        tail_[u] = t;
    }
}


/* --------------------------------------------------------------------------
 * Fast evaluation of a neighbour that swaps two adjacent operations of one
 * machine.  The new machine chain is  p -> v -> u -> s  instead of
 * p -> u -> v -> s, i.e. exactly one operation (u) moves past one operation
 * (v).  A topological order of the new graph is obtained from the current one
 * by moving u just after v.  A single pass over that order yields the longest
 * path (earliest start times).  If the resulting order is not a valid
 * topological order (the move created a cycle, or the order repair failed) we
 * fall back to the full Kahn evaluation.
 * -------------------------------------------------------------------------- */

/* Relax one operation of the neighbour graph; returns -1 if the topological
 * order turns out to be invalid.  b is the machine predecessor in the new
 * graph (v->p, u->v, s->u). */
static inline int relax_one(int x, int *head, int *ms, int u, int v, int s, int p) {
    int a = jpred[x];
    int b = mprev_[x];
    if (x == v) b = p;
    else if (x == u) b = v;
    else if (x == s) b = u;
    if (a >= 0 && !done_[a]) return -1;
    if (b >= 0 && !done_[b]) return -1;
    int h = 0;
    if (a >= 0) { int t = head[a] + dur[a]; if (t > h) h = t; }
    if (b >= 0) { int t = head[b] + dur[b]; if (t > h) h = t; }
    head[x] = h;
    done_[x] = 1;
    int f = h + dur[x];
    if (f > *ms) *ms = f;
    return 0;
}

static int eval_swap_fast(int m, int i, int *head, int *topo) {
    int u = mseq[m][i], v = mseq[m][i + 1];
    int p = (i > 0) ? mseq[m][i - 1] : -1;
    int s = (i + 2 < NJ) ? mseq[m][i + 2] : -1;
    int iu = pos_[u], iv = pos_[v];

    memset(done_, 0, NOPS);
    int ms = 0, r;
    if (iu < iv) {
        /* new order: T[0..iu-1], T[iu+1..iv], u, T[iv+1..] */
        for (int q = 0; q < iu; q++) { r = relax_one(topo_[q], head, &ms, u, v, s, p); if (r < 0) return -2; }
        for (int q = iu + 1; q <= iv; q++) { r = relax_one(topo_[q], head, &ms, u, v, s, p); if (r < 0) return -2; }
        r = relax_one(u, head, &ms, u, v, s, p); if (r < 0) return -2;
        for (int q = iv + 1; q < NOPS; q++) { r = relax_one(topo_[q], head, &ms, u, v, s, p); if (r < 0) return -2; }
    } else {
        /* new order: T[0..iv], u, T[iv+1..iu-1], T[iu+1..] */
        for (int q = 0; q <= iv; q++) { r = relax_one(topo_[q], head, &ms, u, v, s, p); if (r < 0) return -2; }
        r = relax_one(u, head, &ms, u, v, s, p); if (r < 0) return -2;
        for (int q = iv + 1; q < iu; q++) { r = relax_one(topo_[q], head, &ms, u, v, s, p); if (r < 0) return -2; }
        for (int q = iu + 1; q < NOPS; q++) { r = relax_one(topo_[q], head, &ms, u, v, s, p); if (r < 0) return -2; }
    }
    /* record the new topological order (for the accepted move) */
    {
        int k = 0;
        if (iu < iv) {
            for (int q = 0; q < iu; q++) topo[k++] = topo_[q];
            for (int q = iu + 1; q <= iv; q++) topo[k++] = topo_[q];
            topo[k++] = u;
            for (int q = iv + 1; q < NOPS; q++) topo[k++] = topo_[q];
        } else {
            for (int q = 0; q <= iv; q++) topo[k++] = topo_[q];
            topo[k++] = u;
            for (int q = iv + 1; q < iu; q++) topo[k++] = topo_[q];
            for (int q = iu + 1; q < NOPS; q++) topo[k++] = topo_[q];
        }
    }
    return ms;
}


/* --------------------------------------------------------------------------
 * O(1) estimate of the makespan after swapping two adjacent operations of a
 * machine.  H[x] = longest path from the source to the start of x,
 * tail[x] = longest path from the completion of x to the sink, so
 * R[x] = dur[x] + tail[x] is the longest path from the start of x to the sink.
 *
 * After the swap  p -> v -> u -> s  (instead of  p -> u -> v -> s ):
 *   H'(v) = max(H[jp(v)], H[p] + dur[p])
 *   H'(u) = max(H[jp(u)], H'(v) + dur[v])
 *   R'(u) = dur[u] + max(R[s], R[js(u)])
 *   R'(v) = dur[v] + max(R'(u), R[js(v)])
 * The estimate is the longer of the two paths through u and v.  It is used
 * only to *rank* the neighbourhood; the chosen move is always recomputed
 * exactly.
 * -------------------------------------------------------------------------- */
static inline int est_swap(int m, int i) {
    int u = mseq[m][i], v = mseq[m][i + 1];
    int p = mprev_[u], s = mnext_[v];
    int jpv = jpred[v], jpu = jpred[u], jsu = jsucc[u], jsv = jsucc[v];

    int hv = 0;
    if (jpv >= 0) hv = est_[jpv];
    if (p >= 0) { int t = est_[p] + dur[p]; if (t > hv) hv = t; }

    int hu = hv + dur[v];
    if (jpu >= 0 && est_[jpu] > hu) hu = est_[jpu];

    int ru = dur[u];
    if (s >= 0) { int t = dur[u] + dur[s] + tail_[s]; if (t > ru) ru = t; }
    if (jsu >= 0) { int t = dur[u] + dur[jsu] + tail_[jsu]; if (t > ru) ru = t; }

    int rv = dur[v] + ru;
    if (jsv >= 0) { int t = dur[v] + dur[jsv] + tail_[jsv]; if (t > rv) rv = t; }

    int best = hu + ru;
    if (hv + rv > best) best = hv + rv;

    /* the operation that follows v on the machine now waits for u */
    if (s >= 0) {
        int hs = 0;
        if (jsv >= 0) hs = est_[jsv];
        int t = hu + dur[u];
        if (t > hs) hs = t;
        int c = hs + dur[s] + tail_[s];
        if (c > best) best = c;
    }
    /* the job successors of u and v may be delayed as well */
    if (jsu >= 0) {
        int hj = 0;
        int js2 = jsucc[jsu];
        if (js2 >= 0) hj = est_[js2];
        int t = hu + dur[u];
        if (t > hj) hj = t;
        int c = hj + dur[jsu] + tail_[jsu];
        if (c > best) best = c;
    }
    if (jsv >= 0) {
        int hj = 0;
        int js2 = jsucc[jsv];
        if (js2 >= 0) hj = est_[js2];
        int t = hv + dur[v];
        if (t > hj) hj = t;
        int c = hj + dur[jsv] + tail_[jsv];
        if (c > best) best = c;
    }
    return best;
}

static inline void set_pos(void) { for (int t = 0; t < NOPS; t++) pos_[topo_[t]] = t; }
static int cur_eval(void) {
    int ms = eval_kahn(est_, topo_, INT32_MAX);
    if (ms >= 0) set_pos();
    return ms;
}
static void rebuild_links(void) {
    for (int m = 0; m < NM; m++)
        for (int i = 0; i < NJ; i++) {
            int o = mseq[m][i];
            mprev_[o] = (i > 0) ? mseq[m][i - 1] : -1;
            mnext_[o] = (i + 1 < NJ) ? mseq[m][i + 1] : -1;
        }
}

/* --------------------------------------------------------------- moves   */
#define MV_SWAP 0
#define MV_MOVE 1
#define MV_PERM 2

typedef struct {
    int type;
    int m;
    int i, j;      /* swap: i (swap with i+1)   move: from i to j */
    int u, v;      /* the affected operation(s) */
    int key;
} Move;

static Move mv_buf[4096];
static int c_est[4096];
static int perm_buf[NM][NJ];      /* new content of a machine segment       */
static int seg_save[NM][NJ];      /* undo buffer for segment permutations  */

static inline void apply_perm(int m, int i, int j) {
    for (int k = i; k <= j; k++) seg_save[m][k] = mseq[m][k];
    for (int k = i; k <= j; k++) mseq[m][k] = perm_buf[m][k - i];
    rebuild_links();
}
static inline void undo_perm(int m, int i, int j) {
    for (int k = i; k <= j; k++) mseq[m][k] = seg_save[m][k];
    rebuild_links();
}

static inline void apply_swap(int m, int i) {
    int u = mseq[m][i], v = mseq[m][i + 1];
    int p = (i > 0) ? mseq[m][i - 1] : -1;
    int s = (i + 2 < NJ) ? mseq[m][i + 2] : -1;
    mseq[m][i] = v; mseq[m][i + 1] = u;
    mprev_[v] = p; mnext_[v] = u;
    mprev_[u] = v; mnext_[u] = s;
    if (p >= 0) mnext_[p] = v;
    if (s >= 0) mprev_[s] = u;
}

/* Move the op at index i to index j (final index of the moved op is j). */
static void apply_move(int m, int i, int j) {
    int op = mseq[m][i];
    int tmp[NJ];
    int idx = 0;
    for (int k = 0; k < NJ; k++) if (k != i) tmp[idx++] = mseq[m][k];
    for (int k = NJ - 1; k > j; k--) tmp[k] = tmp[k - 1];
    tmp[j] = op;
    memcpy(mseq[m], tmp, sizeof(int) * NJ);
    rebuild_links();
}

/* ------------------------------------------------------------- tabu list */
#define TSIZE (1 << 20)
#define TMASK (TSIZE - 1)
static int tkey[TSIZE], texp[TSIZE];

static inline unsigned t_hash(int key) { return ((unsigned)key * 2654435761u) & TMASK; }

static inline int tabu_get(int key, int iter) {
    unsigned h = t_hash(key);
    for (int p = 0; p < 128; p++) {
        if (tkey[h] == 0) return 0;
        if (tkey[h] == key) return texp[h] > iter;
        h = (h + 1) & TMASK;
    }
    return 0;
}
static void tabu_set(int key, int until) {
    unsigned h = t_hash(key);
    unsigned free_slot = 0; int have_free = 0;
    for (int p = 0; p < 128; p++) {
        if (tkey[h] == 0) { tkey[h] = key; texp[h] = until; return; }
        if (tkey[h] == key) { if (until > texp[h]) texp[h] = until; return; }
        if (!have_free && texp[h] <= until) { free_slot = h; have_free = 1; }
        h = (h + 1) & TMASK;
    }
    if (have_free) { tkey[free_slot] = key; texp[free_slot] = until; }
}
static inline int tabu_remaining(int key, int iter) {
    unsigned h = t_hash(key);
    for (int p = 0; p < 128; p++) {
        if (tkey[h] == 0) return 0;
        if (tkey[h] == key) return texp[h] > iter ? texp[h] - iter : 0;
        h = (h + 1) & TMASK;
    }
    return 0;
}
static void tabu_clear(void) { memset(tkey, 0, sizeof(tkey)); memset(texp, 0, sizeof(texp)); }

/* ------------------------------------------------------------------ I/O  */
static void save_orders(const char *path, const int seq[NM][NJ]) {
    char tmp[4096];
    int len = 0;
    len += snprintf(tmp + len, sizeof(tmp) - len, "{\"machines\": [");
    for (int m = 0; m < NM; m++) {
        len += snprintf(tmp + len, sizeof(tmp) - len, "%s[", m ? ", " : "");
        for (int i = 0; i < NJ; i++)
            len += snprintf(tmp + len, sizeof(tmp) - len, "%s%d", i ? ", " : "", jobof[seq[m][i]]);
        len += snprintf(tmp + len, sizeof(tmp) - len, "]");
    }
    len += snprintf(tmp + len, sizeof(tmp) - len, "]}\n");
    FILE *f = fopen(path, "w");
    if (!f) return;
    fwrite(tmp, 1, len, f);
    fclose(f);
}

static int load_orders(const char *path, int seq[NM][NJ]) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    static char buf[1 << 17];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = 0; fclose(f);
    const char *p = strchr(buf, '[');
    if (!p) return 0;
    p++;
    for (int m = 0; m < NM; m++) {
        while (*p && *p != '[') p++;
        if (!*p) return 0;
        p++;
        int cnt = 0;
        while (*p && *p != ']') {
            while (*p && (*p < '0' || *p > '9')) p++;
            if (!*p) break;
            int job = 0;
            while (*p >= '0' && *p <= '9') { job = job * 10 + (*p - '0'); p++; }
            if (job >= 0 && job < NJ && cnt < NJ) {
                int op = -1;
                for (int k = 0; k < NM; k++)
                    if (ROUTE[job * NM + k] == m) { op = job * NM + k; break; }
                if (op >= 0) seq[m][cnt++] = op;
            }
        }
        if (cnt != NJ) return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ log  */
static FILE *lg;
static volatile sig_atomic_t g_stop = 0;
static void on_sig(int s) { (void)s; g_stop = 1; }
static double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + 1e-9 * ts.tv_nsec;
}
static double cpu_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return ts.tv_sec + 1e-9 * ts.tv_nsec;
}
static void logmsg(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vfprintf(lg, fmt, ap); va_end(ap);
    fflush(lg);
}

/* random feasible perturbation: apply `strength` random adjacent swaps, each
 * one only if the disjunctive graph stays acyclic */
static void kick(int strength, int seq[NM][NJ]) {
    for (int q = 0; q < strength; q++) {
        for (int attempt = 0; attempt < 10; attempt++) {
            int m = (int)rnd(NM), i = (int)rnd(NJ - 1);
            apply_swap(m, i);
            rebuild_links();
            if (eval_kahn(n_est, n_topo, INT32_MAX) >= 0) break;   /* feasible */
            apply_swap(m, i);
            rebuild_links();
        }
    }
}



/* Job re-insertion kick: pick a random job and move each of its operations to
 * a random position on its machine (keeping the schedule acyclic).  This is a
 * much stronger perturbation than a few adjacent swaps. */
static int op_machine[NOPS];

static void kick_job(int njob) {
    for (int q = 0; q < njob; q++) {
        int j = (int)rnd(NJ);
        for (int k = 0; k < NM; k++) {
            int op = j * NM + k;
            int mac = op_machine[op];
            int pos = -1;
            for (int i = 0; i < NJ; i++) if (mseq[mac][i] == op) { pos = i; break; }
            if (pos < 0) continue;
            int newpos = (int)rnd(NJ);
            if (newpos == pos) continue;
            apply_move(mac, pos, newpos);
            rebuild_links();
            if (eval_kahn(n_est, n_topo, INT32_MAX) < 0) {
                apply_move(mac, newpos, pos);
                rebuild_links();
            }
        }
    }
}


/* --------------------------------------------------------------------------
 * Cycle detection ("back-jump" in the i-TSAB sense): remember every solution
 * that has been visited; when the search returns to one of them, jump back to
 * a random elite solution instead of continuing inside the cycle.
 * -------------------------------------------------------------------------- */
#define HSIZE (1 << 20)
static uint64_t htab[HSIZE];

static inline uint64_t hash_sol(void) {
    uint64_t h = 1469598103934665603ULL;
    for (int m = 0; m < NM; m++)
        for (int i = 0; i < NJ; i++) { h ^= (uint64_t)(mseq[m][i] + 1); h *= 1099511628211ULL; }
    return h ? h : 1;
}
/* returns 1 if the solution has been seen before */
static inline int seen_before(uint64_t h) {
    unsigned idx = (unsigned)(h >> 22) & (HSIZE - 1);
    for (int p = 0; p < 96; p++) {
        unsigned k = (idx + p) & (HSIZE - 1);
        if (htab[k] == 0) { htab[k] = h; return 0; }
        if (htab[k] == h) return 1;
    }
    return 0;
}
static void htab_clear(void) { memset(htab, 0, sizeof(htab)); }

/* count distinct solutions that reach the incumbent quality */
#define DTABLE (1 << 18)
static uint64_t dtab[DTABLE];
static long n_distinct_best = 0;
static inline int distinct_best(uint64_t h) {
    unsigned idx = (unsigned)(h >> 24) & (DTABLE - 1);
    for (int p = 0; p < 64; p++) {
        unsigned k = (idx + p) & (DTABLE - 1);
        if (dtab[k] == 0) { dtab[k] = h; return 1; }
        if (dtab[k] == h) return 0;
    }
    return 0;
}

/* ---- elite pool --------------------------------------------------------------- */
#define NELITE 48
static int elite[NELITE][NM][NJ];
static int elite_ms[NELITE];
static int n_elite = 0;

static void elite_add(const int seq[NM][NJ], int ms) {
    if (n_elite == NELITE && ms >= elite_ms[NELITE - 1]) return;
    int pos = (n_elite < NELITE) ? n_elite++ : NELITE - 1;
    while (pos > 0 && elite_ms[pos - 1] > ms) {
        elite_ms[pos] = elite_ms[pos - 1];
        memcpy(elite[pos], elite[pos - 1], sizeof(int) * NM * NJ);
        pos--;
    }
    elite_ms[pos] = ms;
    memcpy(elite[pos], seq, sizeof(int) * NM * NJ);
}
/* pick a random elite, biased towards the better ones */
static int elite_pick(void) {
    if (n_elite <= 1) return 0;
    double u = (double)(rnd64() >> 11) * (1.0 / 9007199254740992.0);
    int r = (int)(-3.0 * log(u > 1e-12 ? u : 1e-12));
    if (r >= n_elite) r = n_elite - 1;
    if (r < 0) r = 0;
    return r;
}


/* --------------------------------------------------------------------------
 * Short tabu burst over the N1 neighbourhood (swap two adjacent operations of
 * one machine).
 *
 * Used on every intermediate solution of a path-relinking bridge: Peng, Lu &
 * Cheng's recipe for the ta43/ta47/ta50 records walks the bridge AND runs a
 * local search at every intermediate step.  The old implementation only
 * *recorded* the intermediates and never improved them.
 *
 * The burst leaves mseq at its best state and returns that makespan.
 * -------------------------------------------------------------------------- */
static int g_btabu_until[NM][NJ];

static int burst_tabu(int iters) {
    int cur = cur_eval();
    if (cur < 0) return cur;
    int best = cur;
    static int bestseq[NM][NJ];
    memcpy(bestseq, mseq, sizeof(bestseq));
    for (int it = 0; it < iters; it++) {
        int bestMs = INT32_MAX, bm = -1, bi = -1;
        for (int m = 0; m < NM; m++)
            for (int i = 0; i + 1 < NJ; i++) {
                if (g_btabu_until[m][i] > it) continue;
                int ms = eval_swap_fast(m, i, n_est, n_topo);
                if (ms == -2) {
                    apply_swap(m, i);
                    ms = eval_kahn(n_est, n_topo, INT32_MAX);
                    apply_swap(m, i);
                }
                if (ms < 0) continue;
                if (ms < bestMs) { bestMs = ms; bm = m; bi = i; }
            }
        if (bm < 0) break;
        apply_swap(bm, bi);
        rebuild_links();
        cur = cur_eval();
        if (cur < 0) break;
        int ten = 6 + (int)rnd(5);
        g_btabu_until[bm][bi] = it + ten;
        g_btabu_until[bm][bi + 1] = it + ten;
        if (cur < best) { best = cur; memcpy(bestseq, mseq, sizeof(bestseq)); }
    }
    memcpy(mseq, bestseq, sizeof(mseq));
    rebuild_links();
    (void)cur_eval();
    return best;
}

/* --------------------------------------------------------------------------
 * Path relinking between two elite solutions: adopt the machine orders of the
 * target one machine at a time (in a random order), keeping every intermediate
 * solution feasible (a machine whose adoption would create a cycle is
 * reverted).  Intermediate solutions can be better than both endpoints.
 * -------------------------------------------------------------------------- */
static void shuffle_machines(int *perm) {
    for (int i = NM - 1; i > 0; i--) { int j = (int)rnd(i + 1); int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
}

static int path_relink(const int target[NM][NJ], int *best_ms_io, int bseq_io[NM][NJ],
                        const char *outfile_io, double t0) {
    int backup[NM][NJ];
    int perm[NM];
    for (int i = 0; i < NM; i++) perm[i] = i;
    shuffle_machines(perm);
    int nnew = 0;
    memcpy(backup, mseq, sizeof(backup));
    for (int t = 0; t < NM; t++) {
        int m = perm[t];
        int save[NM][NJ];
        memcpy(save, mseq, sizeof(save));
        memcpy(mseq[m], target[m], sizeof(int) * NJ);
        rebuild_links();
        int ms = eval_kahn(est_, topo_, INT32_MAX);
        if (ms < 0) {                        /* cycle -> keep the old order */
            memcpy(mseq, save, sizeof(mseq));
            rebuild_links();
            continue;
        }
        nnew++;
        elite_add(mseq, ms);
        if (burst_iters > 0 && (nnew % 3) == 0 && ms <= *best_ms_io + 25) {
            /* local search at every intermediate point of the bridge */
            int keep[NM][NJ];
            memcpy(keep, mseq, sizeof(keep));
            memset(g_btabu_until, 0, sizeof(g_btabu_until));
            int ms2 = burst_tabu(burst_iters);
            if (ms2 >= 0 && ms2 < ms) {
                elite_add(mseq, ms2);
                if (ms2 < *best_ms_io) {
                    *best_ms_io = ms2;
                    memcpy(bseq_io, mseq, sizeof(int) * NM * NJ);
                    save_orders(outfile_io, bseq_io);
                    logmsg("[%8.1fs] PATH RELINK + BURST NEW BEST %d\n", now_sec() - t0, *best_ms_io);
                }
            }
            /* restore the pure bridge state and continue walking */
            memcpy(mseq, keep, sizeof(mseq));
            rebuild_links();
            cur_eval();
        }
        if (ms < *best_ms_io) {
            *best_ms_io = ms;
            memcpy(bseq_io, mseq, sizeof(int) * NM * NJ);
            save_orders(outfile_io, bseq_io);
            logmsg("[%8.1fs] PATH RELINK NEW BEST %d\n", now_sec() - t0, *best_ms_io);
        }
    }
    (void)backup;
    return nnew;
}


/* ====================================================================== */
int main(int argc, char **argv) {
    for (int j = 0; j < NJ; j++)
        for (int k = 0; k < NM; k++) {
            int o = j * NM + k;
            dur[o] = DUR[o];
            jobof[o] = j;
            op_machine[o] = ROUTE[o];
            jpred[o] = (k > 0) ? o - 1 : -1;
            jsucc[o] = (k < NM - 1) ? o + 1 : -1;
        }

    const char *initfile = NULL, *outfile = "best.json", *logfile = "logs/run.log";
    double tl = 0;
    uint64_t seed = 1;
    int nbhd = 0;              /* 0=N1 1=N5 2=N6 3=N8 4=N1+N8 */
    int tt_base = 0;           /* 0 = literature formula */
    int tt_lo = 0, tt_hi = 0;
    int kick_iv = 3000, restart_iv = 15000, pstr = 3, fastmode = 1, selftest = 0, randout = 0;
    int estimode = 0, topk = 5, esttest = 0, nbridge = 0, kicktype = 0, tie_rand = 0, kick_trig = 0, multistart = 0, backjump = 0;
    long n_backjump = 0, last_backjump = -1000;
    double cpu_limit = 0; long maxiter = 0;
    long n_est_test = 0, n_hit = 0, n_hit1 = 0; double n_regret_sum = 0, n_true_sum = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) initfile = argv[++i];
        else if (!strcmp(argv[i], "-o") && i + 1 < argc) outfile = argv[++i];
        else if (!strcmp(argv[i], "-l") && i + 1 < argc) logfile = argv[++i];
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) tl = atof(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--nb") && i + 1 < argc) nbhd = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--tt") && i + 2 < argc) { tt_lo = atoi(argv[++i]); tt_hi = atoi(argv[++i]); }
        else if (!strcmp(argv[i], "--kick") && i + 1 < argc) kick_iv = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--restart") && i + 1 < argc) restart_iv = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--pstr") && i + 1 < argc) pstr = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--fast") && i + 1 < argc) fastmode = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--selftest")) selftest = 1;
        else if (!strcmp(argv[i], "--randout") && i + 1 < argc) randout = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--est") && i + 1 < argc) estimode = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--topk") && i + 1 < argc) topk = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--esttest")) esttest = 1;
        else if (!strcmp(argv[i], "--bridge") && i + 1 < argc) nbridge = atoi(argv[++i]);
            else if (!strcmp(argv[i], "--burst") && i + 1 < argc) burst_iters = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--kicktype") && i + 1 < argc) kicktype = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--tierand")) tie_rand = 1;
        else if (!strcmp(argv[i], "--kicktrig") && i + 1 < argc) kick_trig = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--multistart") && i + 1 < argc) multistart = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--backjump")) backjump = 1;
        else if (!strcmp(argv[i], "--cpu") && i + 1 < argc) cpu_limit = atof(argv[++i]);
        else if (!strcmp(argv[i], "--maxiter") && i + 1 < argc) maxiter = atol(argv[++i]);
        else { fprintf(stderr, "unknown arg %s\n", argv[i]); return 2; }
    }
    (void)tt_base;

    if (randout) {
        if (!initfile || !load_orders(initfile, mseq)) { fprintf(stderr, "no init\n"); return 1; }
        rebuild_links();
        int ms = cur_eval();
        for (int q = 0; q < randout; q++) {
            int m = (int)rnd(NM), i = (int)rnd(NJ - 1);
            apply_swap(m, i);
            rebuild_links();
            int t = eval_kahn(est_, topo_, INT32_MAX);
            if (t < 0) { apply_swap(m, i); rebuild_links(); }
        }
        ms = cur_eval();
        save_orders(outfile, mseq);
        printf("wrote %s makespan %d\n", outfile, ms);
        return 0;
    }

    if (selftest) {
        if (!initfile || !load_orders(initfile, mseq)) { fprintf(stderr, "no init\n"); return 1; }
        rebuild_links();
        int ms = cur_eval();
        printf("initial makespan %d\n", ms);
        int bad = 0, fb = 0, n = 20000;
        uint64_t rs = 12345;
        for (int it = 0; it < n; it++) {
            rs = rs * 6364136223846793005ULL + 1442695040888963407ULL;
            int m = (int)((rs >> 33) % NM);
            rs = rs * 6364136223846793005ULL + 1442695040888963407ULL;
            int i = (int)((rs >> 33) % (NJ - 1));
            int f = eval_swap_fast(m, i, n_est, n_topo);
            apply_swap(m, i);
            int k = eval_kahn(n_est, n_topo, INT32_MAX);
            apply_swap(m, i);
            if (f == -2) { fb++; continue; }
            if (f != k) { bad++; if (bad < 6) printf("MISMATCH m=%d i=%d fast=%d kahn=%d\n", m, i, f, k); }
        }
        printf("selftest: %d trials, %d mismatches, %d fallbacks\n", n, bad, fb);
        return bad ? 1 : 0;
    }

    signal(SIGTERM, on_sig);
    signal(SIGINT, on_sig);
    lg = fopen(logfile, "a");
    setvbuf(lg, NULL, _IOLBF, 0);
    if (!lg) lg = stderr;
    rng_s = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    for (int i = 0; i < 20; i++) rnd64();

    /* tabu tenure: L = 10 + jobs/machines ; Lmin=round(L), Lmax=round(1.4L) */
    double L = 10.0 + (double)NJ / (double)NM;
    int tt_min = tt_lo ? tt_lo : (int)(L + 0.5);
    int tt_max = tt_hi ? tt_hi : (int)(1.4 * L + 0.5);
    if (tt_max < tt_min) tt_max = tt_min;

    static int bseq[NM][NJ];
    if (!initfile || !load_orders(initfile, mseq)) {
        for (int m = 0; m < NM; m++)
            for (int i = 0; i < NJ; i++) {
                int j = (i + m) % NJ, op = -1;
                for (int k = 0; k < NM; k++) if (ROUTE[j * NM + k] == m) { op = j * NM + k; break; }
                mseq[m][i] = op;
            }
    }
    memcpy(bseq, mseq, sizeof(bseq));
    rebuild_links();
    int cur_ms = cur_eval();
    if (cur_ms < 0) { logmsg("[init] cyclic start!\n"); return 1; }
    int best_ms = cur_ms;
    logmsg("[init] seed=%llu nb=%d makespan=%d tt=[%d,%d]\n",
           (unsigned long long)seed, nbhd, cur_ms, tt_min, tt_max);
    calc_tails(topo_, NOPS);
    for (int o = 0; o < NOPS; o++) pos_[topo_[o]] = 0;
    for (int t = 0; t < NOPS; t++) pos_[topo_[t]] = t;
    elite_add(mseq, best_ms);
    save_orders(outfile, bseq);          /* always keep a valid certificate on disk */

    long iter = 0, stall = 0, kicks = 0, restarts = 0, n_improve = 0, n_kick_run = 0;
    int pstr_eff = pstr;
    long n_cand_tot = 0, n_eval_tot = 0, n_empty = 0;
    double t0 = now_sec();
    int nmoves;

    /* critical blocks: for each machine, runs of consecutive critical ops */
    int blk_start[NM][NJ], blk_len[NM][NJ], nblk[NM];

    while (1) {
        if (cpu_limit > 0 && (iter & 63) == 0 && cpu_sec() > cpu_limit) break;
        if (maxiter > 0 && iter >= maxiter) break;
        if (g_stop) { logmsg("[%8.1fs] signal -> stopping, saving best %d\n", now_sec() - t0, best_ms); break; }
        if (tl > 0 && (iter & 127) == 0 && now_sec() - t0 > tl) break;

        /* ---- critical flags ---- */
        for (int o = 0; o < NOPS; o++)
            crit[o] = (unsigned char)(est_[o] + dur[o] + tail_[o] == cur_ms);

        /* ---- build the move list ---- */
        nmoves = 0;
        if (nbhd & 1) {                        /* N1 */
            for (int m = 0; m < NM; m++)
                for (int i = 0; i + 1 < NJ; i++) {
                    int u = mseq[m][i], v = mseq[m][i + 1];
                    if (!crit[u] || !crit[v]) continue;
                    Move *mv = &mv_buf[nmoves++];
                    mv->type = MV_SWAP; mv->m = m; mv->i = i; mv->j = i + 1;
                    mv->u = u; mv->v = v;
                    int a = u < v ? u : v, b = u < v ? v : u;
                    mv->key = a * NOPS + b + 1;
                }
        }
        /* critical blocks */
        for (int m = 0; m < NM; m++) {
            nblk[m] = 0;
            int i = 0;
            while (i < NJ) {
                if (!crit[mseq[m][i]]) { i++; continue; }
                int st = i;
                while (i < NJ && crit[mseq[m][i]]) i++;
                blk_start[m][nblk[m]] = st;
                blk_len[m][nblk[m]] = i - st;
                nblk[m]++;
            }
        }
        if (nbhd & 2) {                        /* N5 */
            for (int m = 0; m < NM; m++)
                for (int b = 0; b < nblk[m]; b++) {
                    int st = blk_start[m][b], ln = blk_len[m][b];
                    if (ln >= 2) {
                        int u = mseq[m][st], v = mseq[m][st + 1];
                        Move *mv = &mv_buf[nmoves++];
                        mv->type = MV_SWAP; mv->m = m; mv->i = st; mv->j = st + 1;
                        mv->u = u; mv->v = v;
                        int a = u < v ? u : v, b2 = u < v ? v : u;
                        mv->key = a * NOPS + b2 + 1;
                    }
                    if (ln >= 3) {
                        int u = mseq[m][st + ln - 2], v = mseq[m][st + ln - 1];
                        Move *mv = &mv_buf[nmoves++];
                        mv->type = MV_SWAP; mv->m = m; mv->i = st + ln - 2; mv->j = st + ln - 1;
                        mv->u = u; mv->v = v;
                        int a = u < v ? u : v, b2 = u < v ? v : u;
                        mv->key = a * NOPS + b2 + 1;
                    }
                }
        }
        if (nbhd & 4) {                        /* N6 : move op to block ends */
            for (int m = 0; m < NM; m++)
                for (int b = 0; b < nblk[m]; b++) {
                    int st = blk_start[m][b], ln = blk_len[m][b];
                    if (ln < 3) continue;   /* a block of 2 gives only the adjacent swap (already in N1) */
                    for (int k = 1; k < ln; k++) {
                        int i = st + k, u = mseq[m][i];
                        Move *mv = &mv_buf[nmoves++];
                        mv->type = MV_MOVE; mv->m = m; mv->i = i; mv->j = st;
                        mv->u = u; mv->v = -1;
                        mv->key = NOPS * NOPS + m * NOPS + u + 1;
                    }
                    for (int k = 0; k < ln - 1; k++) {
                        int i = st + k, u = mseq[m][i];
                        Move *mv = &mv_buf[nmoves++];
                        mv->type = MV_MOVE; mv->m = m; mv->i = i; mv->j = st + ln - 1;
                        mv->u = u; mv->v = -1;
                        mv->key = NOPS * NOPS + m * NOPS + u + 1;
                    }
                }
        }
        if (nbhd & 16) {                       /* coordinated critical-block moves */
            for (int m = 0; m < NM; m++)
                for (int b = 0; b < nblk[m]; b++) {
                    int st = blk_start[m][b], ln = blk_len[m][b];
                    if (ln < 2) continue;
                    /* (a) reverse the block */
                    for (int k = 0; k < ln; k++) perm_buf[m][k] = mseq[m][st + ln - 1 - k];
                    Move *mv = &mv_buf[nmoves++];
                    mv->type = MV_PERM; mv->m = m; mv->i = st; mv->j = st + ln - 1;
                    mv->u = mseq[m][st]; mv->v = -1;
                    mv->key = NOPS * NOPS * 2 + m * NOPS + st + 1;
                    /* (b) rotate left: first op of the block goes to the end */
                    for (int k = 0; k < ln - 1; k++) perm_buf[m][k] = mseq[m][st + k + 1];
                    perm_buf[m][ln - 1] = mseq[m][st];
                    mv = &mv_buf[nmoves++];
                    mv->type = MV_PERM; mv->m = m; mv->i = st; mv->j = st + ln - 1;
                    mv->u = mseq[m][st]; mv->v = -1;
                    mv->key = NOPS * NOPS * 2 + m * NOPS + st + 2;
                    /* (c) rotate right: last op of the block goes to the front */
                    for (int k = 0; k < ln - 1; k++) perm_buf[m][k] = mseq[m][st + k];
                    perm_buf[m][ln - 1] = mseq[m][st + ln - 1];
                    mv = &mv_buf[nmoves++];
                    mv->type = MV_PERM; mv->m = m; mv->i = st; mv->j = st + ln - 1;
                    mv->u = mseq[m][st + ln - 1]; mv->v = -1;
                    mv->key = NOPS * NOPS * 2 + m * NOPS + st + 3;
                }
        }
        if (nbhd & 8) {                        /* N8 : move inner op outside block */
            for (int m = 0; m < NM; m++)
                for (int b = 0; b < nblk[m]; b++) {
                    int st = blk_start[m][b], ln = blk_len[m][b];
                    if (ln < 3) continue;
                    for (int k = 1; k < ln - 1; k++) {
                        int i = st + k, u = mseq[m][i];
                        /* before the block */
                        if (st > 0) {
                            Move *mv = &mv_buf[nmoves++];
                            mv->type = MV_MOVE; mv->m = m; mv->i = i; mv->j = st - 1;
                            mv->u = u; mv->v = -1;
                            mv->key = NOPS * NOPS + m * NOPS + u + 1;
                        }
                        /* after the block */
                        if (st + ln < NJ) {
                            Move *mv = &mv_buf[nmoves++];
                            mv->type = MV_MOVE; mv->m = m; mv->i = i; mv->j = st + ln;
                            mv->u = u; mv->v = -1;
                            mv->key = NOPS * NOPS + m * NOPS + u + 1;
                        }
                    }
                }
        }
        n_cand_tot += nmoves;

        if (nmoves == 0) {
            /* local optimum w.r.t. this neighbourhood: perturb and go on */
            stall++;
            n_empty++;
            if (stall > 20) {
                memcpy(mseq, bseq, sizeof(mseq));
                rebuild_links();
                tabu_clear();
                stall = 0;
            }
            kick(pstr, mseq);
            cur_ms = cur_eval();
            if (cur_ms < 0) {
                memcpy(mseq, bseq, sizeof(mseq));
                rebuild_links();
                cur_ms = cur_eval();
            }
            calc_tails(topo_, NOPS);
            continue;
        }

        /* ---- evaluate the neighbourhood ---- */
        int bestMoveMs = INT32_MAX, bestMoveIdx = -1, bestKey = 0;
        int leastTabuRem = INT32_MAX, leastTabuIdx = -1;
        int n_ties = 0, tie_list[64];

        if (estimode) {
            /* rank with the O(1) estimate, then recompute the top-K exactly */
            int cand_idx[64], k = 0;
            for (int c = 0; c < nmoves; c++) {
                Move *mv = &mv_buf[c];
                if (mv->type != MV_SWAP) continue;           /* insertions: exact only */
                int e = est_swap(mv->m, mv->i);
                c_est[c] = e;
                /* insert into the sorted top-K list */
                if (k < topk) {
                    int q = k++;
                    while (q > 0 && c_est[cand_idx[q - 1]] > e) { cand_idx[q] = cand_idx[q - 1]; q--; }
                    cand_idx[q] = c;
                } else if (e < c_est[cand_idx[k - 1]]) {
                    int q = k - 1;
                    while (q > 0 && c_est[cand_idx[q - 1]] > e) { cand_idx[q] = cand_idx[q - 1]; q--; }
                    cand_idx[q] = c;
                }
            }
            /* evaluate the K best-by-estimate candidates exactly */
            if (esttest) {
                int truebest = INT32_MAX, truebestc = -1;
                for (int c = 0; c < nmoves; c++) {
                    Move *mv = &mv_buf[c];
                    if (mv->type != MV_SWAP) continue;
                    apply_swap(mv->m, mv->i);
                    int ms = eval_kahn(n_est, n_topo, INT32_MAX);
                    apply_swap(mv->m, mv->i);
                    if (ms >= 0 && ms < truebest) { truebest = ms; truebestc = c; }
                }
                /* the candidate the estimate would pick */
                int pick = cand_idx[0];
                if (pick >= 0) {
                    Move *mv = &mv_buf[pick];
                    apply_swap(mv->m, mv->i);
                    int ms = eval_kahn(n_est, n_topo, INT32_MAX);
                    apply_swap(mv->m, mv->i);
                    n_est_test++;
                    if (ms >= 0) {
                        n_regret_sum += (ms - truebest);
                        if (ms == truebest) n_hit++;
                        if (truebestc >= 0 && pick == truebestc) n_hit1++;
                    }
                }
                n_true_sum += truebest;
            }
            for (int q = 0; q < k; q++) {
                int c = cand_idx[q];
                Move *mv = &mv_buf[c];
                apply_swap(mv->m, mv->i);
                int ms = eval_kahn(n_est, n_topo, bestMoveMs - 1);
                apply_swap(mv->m, mv->i);
                n_eval_tot++;
                if (ms < 0) continue;
                int isTabu = tabu_get(mv->key, (int)iter);
                if (isTabu && !(ms < best_ms)) continue;
                if (ms < bestMoveMs) {
                    bestMoveMs = ms; bestMoveIdx = c; bestKey = mv->key;
                    memcpy(est_, n_est, sizeof(est_));
                    memcpy(topo_, n_topo, sizeof(topo_));
                }
            }
        } else {
        for (int c = 0; c < nmoves; c++) {
            Move *mv = &mv_buf[c];
            int ms;
            if (mv->type == MV_SWAP && fastmode) {
                ms = eval_swap_fast(mv->m, mv->i, n_est, n_topo);
                last_aborted = 0;
                if (ms == -2) {
                    apply_swap(mv->m, mv->i);
                    ms = eval_kahn(n_est, n_topo, bestMoveMs - 1);
                    apply_swap(mv->m, mv->i);
                }
            } else if (mv->type == MV_SWAP) {
                apply_swap(mv->m, mv->i);
                ms = eval_kahn(n_est, n_topo, bestMoveMs - 1);
                apply_swap(mv->m, mv->i);
            } else if (mv->type == MV_PERM) {
                apply_perm(mv->m, mv->i, mv->j);
                ms = eval_kahn(n_est, n_topo, bestMoveMs - 1);
                undo_perm(mv->m, mv->i, mv->j);
            } else {
                apply_move(mv->m, mv->i, mv->j);
                ms = eval_kahn(n_est, n_topo, bestMoveMs - 1);
                apply_move(mv->m, mv->j, mv->i);
            }
            n_eval_tot++;
            if (ms < 0) continue;
            int isTabu = tabu_get(mv->key, (int)iter);
            if (isTabu) {
                /* remember the least tabu candidate as a fallback */
                int rem = tabu_remaining(mv->key, (int)iter);
                if (rem < leastTabuRem) { leastTabuRem = rem; leastTabuIdx = c; }
                if (!(ms < best_ms)) continue;              /* aspiration */
            }
            if (ms < bestMoveMs) {
                bestMoveMs = ms; bestMoveIdx = c; bestKey = mv->key;
                memcpy(est_, n_est, sizeof(est_));
                memcpy(topo_, n_topo, sizeof(topo_));
                n_ties = 0; tie_list[0] = c;
            } else if (ms == bestMoveMs && n_ties < 64 && !last_aborted) {
                tie_list[n_ties++] = c;
            }
        }
        if (n_ties > 1 && tie_rand) {
            int pick = tie_list[(int)rnd((uint32_t)n_ties)];
            Move *mv = &mv_buf[pick];
            if (mv->type == MV_SWAP) { apply_swap(mv->m, mv->i); cur_ms = eval_kahn(est_, topo_, INT32_MAX); }
            else if (mv->type == MV_MOVE) { apply_move(mv->m, mv->i, mv->j); cur_ms = eval_kahn(est_, topo_, INT32_MAX); }
            else { apply_perm(mv->m, mv->i, mv->j); cur_ms = eval_kahn(est_, topo_, INT32_MAX); }
            bestKey = mv->key;
            goto after_move;
        }
        }

        if (bestMoveIdx < 0 && leastTabuIdx >= 0) {
            /* everything tabu: take the move whose tabu expires soonest */
            int c = leastTabuIdx;
            Move *mv = &mv_buf[c];
            if (mv->type == MV_SWAP) {
                apply_swap(mv->m, mv->i);
                cur_ms = cur_eval();
            } else if (mv->type == MV_MOVE) {
                apply_move(mv->m, mv->i, mv->j);
                cur_ms = cur_eval();
            } else {
                apply_perm(mv->m, mv->i, mv->j);
                cur_ms = cur_eval();
            }
            bestKey = mv->key;
        } else if (bestMoveIdx < 0) {
            /* everything tabu: take a random move */
            int c = (int)rnd(nmoves);
            Move *mv = &mv_buf[c];
            if (mv->type == MV_SWAP) {
                apply_swap(mv->m, mv->i);
                cur_ms = cur_eval();
                if (cur_ms < 0) { apply_swap(mv->m, mv->i); cur_ms = cur_eval(); }
            } else {
                apply_move(mv->m, mv->i, mv->j);
                cur_ms = cur_eval();
                if (cur_ms < 0) { apply_move(mv->m, mv->j, mv->i); cur_ms = cur_eval(); }
            }
            bestKey = mv->key;
        } else {
            Move *mv = &mv_buf[bestMoveIdx];
            if (mv->type == MV_SWAP) apply_swap(mv->m, mv->i);
            else if (mv->type == MV_MOVE) apply_move(mv->m, mv->i, mv->j);
            else apply_perm(mv->m, mv->i, mv->j);
            cur_ms = bestMoveMs;
        }
after_move:
        tabu_set(bestKey, (int)(iter + tt_min + (int)rnd((uint32_t)(tt_max - tt_min + 1))));

        iter++;
        calc_tails(topo_, NOPS);
        for (int t = 0; t < NOPS; t++) pos_[topo_[t]] = t;
        if (backjump && n_elite > 1 && iter - last_backjump > 300 && seen_before(hash_sol())) {
            /* we are cycling: jump back to a random elite */
            int pick = elite_pick();
            memcpy(mseq, elite[pick], sizeof(mseq));
            rebuild_links();
            cur_ms = cur_eval();
            calc_tails(topo_, NOPS);
            for (int t = 0; t < NOPS; t++) pos_[topo_[t]] = t;
            n_backjump++;
            last_backjump = iter;
            tabu_clear();
        }

        if ((iter % 50000) == 0) {
            if (cur_ms == best_ms && distinct_best(hash_sol())) n_distinct_best++;
            logmsg("[%8.1fs cpu=%7.1fs iter=%9ld] cur=%d best=%d stall=%ld kicks=%ld restarts=%ld elites=%d evals=%ld distinct_best=%ld\n",
                   now_sec() - t0, cpu_sec(), iter, cur_ms, best_ms, stall, kicks, restarts, n_elite, n_eval_tot, n_distinct_best);
        }
        if (cur_ms >= 0 && cur_ms < best_ms) {
            best_ms = cur_ms;
            memcpy(bseq, mseq, sizeof(bseq));
            elite_add(mseq, cur_ms);
            stall = 0; n_improve++;
            save_orders(outfile, bseq);
            logmsg("[%8.1fs iter=%9ld] NEW BEST %d\n", now_sec() - t0, iter, best_ms);
        } else stall++;

        /* ---- perturbation ---- */
        if (stall > 0 && stall % kick_iv == 0 && (kick_trig <= 0 || cur_ms > best_ms + kick_trig)) {
            kicks++;
            if (n_kick_run % 20 == 0) pstr_eff = pstr + 1 + (n_kick_run / 20);
            if (pstr_eff > 12) pstr_eff = 12;
            if (kicktype == 0) kick(pstr_eff, mseq);
            else if (kicktype == 1) { kick_job(1); kick(pstr_eff, mseq); }
            else { if (rnd(100) < 50) kick_job(1); else kick(pstr_eff, mseq); }
            n_kick_run++;
            cur_ms = cur_eval();
            calc_tails(topo_, NOPS);
            elite_add(mseq, cur_ms);
            n_kick_run++;
        }
        if (stall > 0 && stall % restart_iv == 0) {
            restarts++;
            n_kick_run = 0; pstr_eff = pstr;
            int pick = (rnd(100) < 70) ? -1 : elite_pick();
            if (pick < 0) memcpy(mseq, bseq, sizeof(mseq));
            else memcpy(mseq, elite[pick], sizeof(mseq));
            rebuild_links();
            if (multistart > 0) {
                /* diversify: perturb the incumbent with random feasible swaps so
                 * that the next descent lands in a different basin */
                kick(multistart, mseq);
            }
            cur_ms = cur_eval();
            calc_tails(topo_, NOPS);
            /* Feed the solution we are restarting FROM into the elite pool.
             * Without this the pool only ever holds improvements of the
             * incumbent, so on a record-holding instance (2006) it stays at
             * size 1 and the bridge never runs. */
            elite_add(mseq, cur_ms);
            stall = 0;
            tabu_clear();
            /* path relinking towards a random elite */
            if (nbridge > 0 && n_elite > 1) {
                for (int bq = 0; bq < nbridge; bq++) {
                    int tgt = elite_pick();
                    if (tgt == pick) tgt = (tgt + 1) % n_elite;
                    path_relink(elite[tgt], &best_ms, bseq, outfile, t0);
                }
                memcpy(mseq, bseq, sizeof(mseq));
                rebuild_links();
                cur_ms = cur_eval();
                calc_tails(topo_, NOPS);
            }
        }
    }

    save_orders(outfile, bseq);
    if (esttest && n_est_test) {
        logmsg("[esttest] samples=%ld  estimate-picks-the-exact-best=%.1f%%  regret=%.3f  true_best_avg=%.2f\n",
               n_est_test, 100.0 * n_hit / n_est_test, n_regret_sum / n_est_test, n_true_sum / n_est_test);
    }
    logmsg("[done] iters=%ld kicks=%ld restarts=%ld backjumps=%ld improves=%ld empty=%ld cand/iter=%.1f best=%d elapsed=%.1fs\n",
           iter, kicks, restarts, n_backjump, n_improve, n_empty, iter ? (double)n_cand_tot / iter : 0.0,
           best_ms, now_sec() - t0);
    fclose(lg);
    return 0;
}
