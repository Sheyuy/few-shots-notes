#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define NJ 30
#define NM 20
#define NOPS (NJ*NM)
#define INF (1 << 30)

static int route[NJ][NM];
static int dur[NJ][NM];
static int pos[NJ][NM];
static int orders[NM][NJ + 4];
static int orderpos[NM][NJ + 4];
static int len[NM];

static int jsucc[NOPS], msucc[NOPS], indeg[NOPS];
static int start[NOPS], remaining[NOPS], topo[NOPS];
static int crit[NOPS];

static int makespan(int *critmask) {
    for (int v = 0; v < NOPS; v++) { jsucc[v] = -1; msucc[v] = -1; indeg[v] = 0; start[v] = 0; }
    for (int j = 0; j < NJ; j++)
        for (int k = 0; k < NM - 1; k++) { int u = j * NM + k, w = j * NM + k + 1; jsucc[u] = w; indeg[w]++; }
    for (int m = 0; m < NM; m++)
        for (int p = 0; p < len[m] - 1; p++) {
            int a = orders[m][p], b = orders[m][p + 1];
            int u = a * NM + pos[a][m], v = b * NM + pos[b][m];
            msucc[u] = v; indeg[v]++;
        }
    int done = 0;
    int stack[NOPS]; int sp = 0;
    for (int v = 0; v < NOPS; v++) if (indeg[v] == 0) stack[sp++] = v;
    while (sp > 0) {
        int u = stack[--sp]; topo[done++] = u;
        int fin = start[u] + dur[u / NM][u % NM];
        if (jsucc[u] >= 0) { if (fin > start[jsucc[u]]) start[jsucc[u]] = fin; if (--indeg[jsucc[u]] == 0) stack[sp++] = jsucc[u]; }
        if (msucc[u] >= 0) { if (fin > start[msucc[u]]) start[msucc[u]] = fin; if (--indeg[msucc[u]] == 0) stack[sp++] = msucc[u]; }
    }
    if (done != NOPS) return -1;
    int makesp = 0;
    for (int v = 0; v < NOPS; v++) { int fin = start[v] + dur[v / NM][v % NM]; if (fin > makesp) makesp = fin; }
    for (int idx = NOPS - 1; idx >= 0; idx--) {
        int u = topo[idx]; int r = dur[u / NM][u % NM];
        if (jsucc[u] >= 0) { int t = dur[u / NM][u % NM] + remaining[jsucc[u]]; if (t > r) r = t; }
        if (msucc[u] >= 0) { int t = dur[u / NM][u % NM] + remaining[msucc[u]]; if (t > r) r = t; }
        remaining[u] = r;
    }
    for (int v = 0; v < NOPS; v++) if (critmask) critmask[v] = (start[v] + remaining[v] == makesp) ? 1 : 0;
    return makesp;
}

static void remove_at(int m, int job) {
    int p = orderpos[m][job];
    for (int i = p; i < len[m] - 1; i++) orders[m][i] = orders[m][i + 1];
    len[m]--;
    for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i;
}
static void insert_at(int m, int job, int p) {
    for (int i = len[m]; i > p; i--) orders[m][i] = orders[m][i - 1];
    orders[m][p] = job; len[m]++;
    for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i;
}

/* critical-path insertion descent (N4 neighborhood) */
static void descend_insert(int maxiter) {
    for (int it = 0; it < maxiter; it++) {
        int cur = makespan(crit);
        int crit_ops[NOPS]; int ncrit = 0;
        for (int v = 0; v < NOPS; v++) if (crit[v]) crit_ops[ncrit++] = v;
        int best_ms = INF, bm = -1, bt = -1, bj = -1;
        for (int ci = 0; ci < ncrit; ci++) {
            int v = crit_ops[ci]; int j = v / NM, k = v % NM; int m = route[j][k]; int p = orderpos[m][j];
            for (int t = 0; t < len[m]; t++) {
                if (t == p) continue;
                remove_at(m, j); insert_at(m, j, t);
                int ms = makespan(crit);
                remove_at(m, j); insert_at(m, j, p);
                if (ms >= 0 && ms < best_ms) { best_ms = ms; bm = m; bt = t; bj = j; }
            }
        }
        if (best_ms < cur) { remove_at(bm, bj); insert_at(bm, bj, bt); }
        else break;
    }
}

/* critical-block descent (N5 shift / N6 reversal neighborhood) */
static void descend_block(int maxiter) {
    for (int it = 0; it < maxiter; it++) {
        int cur = makespan(crit);
        int bmach[NM * NJ], bstart[NM * NJ], bend[NM * NJ], nb = 0;
        for (int m = 0; m < NM; m++) {
            int p = 0;
            while (p < len[m]) {
                int jp = orders[m][p];
                if (crit[jp * NM + pos[jp][m]]) {
                    int a = p;
                    while (p < len[m]) { int jq = orders[m][p]; if (!crit[jq * NM + pos[jq][m]]) break; p++; }
                    int b = p - 1;
                    if (b - a >= 1) { bmach[nb] = m; bstart[nb] = a; bend[nb] = b; nb++; }
                } else p++;
            }
        }
        int improved = 0;
        for (int bi = 0; bi < nb; bi++) {
            int m = bmach[bi], a = bstart[bi], b = bend[bi], L = b - a + 1;
            int seg[NJ];
            for (int i = 0; i < L; i++) seg[i] = orders[m][a + i];
            int applied = -1;
            for (int type = 0; type < 3; type++) {
                for (int i = 0; i < L; i++) {
                    int src = (type == 0) ? seg[(i + 1) % L] : (type == 1) ? seg[(i + L - 1) % L] : seg[L - 1 - i];
                    orders[m][a + i] = src;
                }
                int ms = makespan(crit);
                if (ms >= 0 && ms < cur) { cur = ms; applied = type; break; }
                for (int i = 0; i < L; i++) orders[m][a + i] = seg[i]; /* restore */
            }
            if (applied >= 0) { for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i; improved = 1; break; }
        }
        if (!improved) break;
    }
}

static void local_search(void) {
    int before, after;
    int guard = 0;
    do {
        before = makespan(NULL);
        descend_insert(300);
        descend_block(80);
        after = makespan(NULL);
    } while (after < before && ++guard < 10);
}

/* greedy reinsertion of one job at its best position on each machine */
static int insert_job(int g) {
    for (int k = 0; k < NM; k++) {
        int m = route[g][k];
        if (orderpos[m][g] < len[m] && orders[m][orderpos[m][g]] == g) remove_at(m, g);
        int best_p = 0, best_ms = 1 << 30;
        int L = len[m];
        for (int p = 0; p <= L; p++) {
            insert_at(m, g, p);
            int ms = makespan(NULL);
            if (ms >= 0 && ms < best_ms) { best_ms = ms; best_p = p; }
            remove_at(m, g);
        }
        insert_at(m, g, best_p);
    }
    return makespan(NULL);
}

/* IG-style kick: remove d random jobs, reinsert greedily, light descent */
static int kick(int d) {
    int removed[NJ];
    for (int i = 0; i < d; i++) {
        int j;
        do { j = rand() % NJ; } while (i > 0 && (j == removed[0] || (i > 1 && j == removed[1]) || (i > 2 && j == removed[2]) || (i > 3 && j == removed[3]) || (i > 4 && j == removed[4])));
        removed[i] = j;
    }
    for (int i = 0; i < d; i++) for (int m = 0; m < NM; m++) if (orderpos[m][removed[i]] < len[m] && orders[m][orderpos[m][removed[i]]] == removed[i]) remove_at(m, removed[i]);
    for (int i = 0; i < d; i++) insert_job(removed[i]);
    for (int it = 0; it < 200; it++) {
        int c = makespan(crit);
        int crit_ops[NOPS]; int ncrit = 0;
        for (int v = 0; v < NOPS; v++) if (crit[v]) crit_ops[ncrit++] = v;
        int best_ms2 = 1 << 30, bm = -1, bt = -1, bj = -1;
        for (int ci = 0; ci < ncrit; ci++) {
            int v = crit_ops[ci]; int j = v / NM, k = v % NM; int m = route[j][k]; int p = orderpos[m][j];
            for (int t = 0; t < len[m]; t++) {
                if (t == p) continue;
                remove_at(m, j); insert_at(m, j, t);
                int ms = makespan(crit);
                remove_at(m, j); insert_at(m, j, p);
                if (ms >= 0 && ms < best_ms2) { best_ms2 = ms; bm = m; bt = t; bj = j; }
            }
        }
        if (best_ms2 < c) { remove_at(bm, bj); insert_at(bm, bj, bt); }
        else break;
    }
    return makespan(NULL);
}

/* SA improvver over the global `orders` (insertion + N5/N6 block + IG kicks).
   This is the component proven to reach the 2006 basin. */
static double SA_T0 = 150.0, SA_Tend = 0.05;
static int SA_KICK = 5;
static int LS_ITER = 1000000;

static int sa_improve(int maxiter) {
    int since = 0;
    for (int iter = 0; iter < maxiter; iter++) {
        double frac = (double)iter / (double)maxiter;
        double T = SA_T0 * pow(SA_Tend / SA_T0, frac);
        int c = makespan(crit);
        if (c < 0) { kick(SA_KICK); since = 0; continue; }
        int crit_ops[NOPS]; int ncrit = 0;
        for (int v = 0; v < NOPS; v++) if (crit[v]) crit_ops[ncrit++] = v;
        if (ncrit == 0) { kick(SA_KICK); since = 0; continue; }
        int bmachine[NM * NJ], bstart[NM * NJ], bend[NM * NJ], nblocks = 0;
        for (int m = 0; m < NM; m++) {
            int p = 0;
            while (p < len[m]) {
                int jp = orders[m][p];
                if (crit[jp * NM + pos[jp][m]]) {
                    int a = p;
                    while (p < len[m]) { int jq = orders[m][p]; if (!crit[jq * NM + pos[jq][m]]) break; p++; }
                    int b = p - 1;
                    if (b - a >= 1) { bmachine[nblocks] = m; bstart[nblocks] = a; bend[nblocks] = b; nblocks++; }
                } else p++;
            }
        }
        int accept = 0;
        int use_block = (nblocks > 0 && rand() % 100 < 50);
        if (use_block) {
            int bi = rand() % nblocks;
            int m = bmachine[bi], a = bstart[bi], b = bend[bi], L = b - a + 1;
            int seg[NJ];
            for (int i = 0; i < L; i++) seg[i] = orders[m][a + i];
            int type = rand() % 3;
            for (int i = 0; i < L; i++) {
                int src = (type == 0) ? seg[(i + 1) % L] : (type == 1) ? seg[(i + L - 1) % L] : seg[L - 1 - i];
                orders[m][a + i] = src;
            }
            int new_ms = makespan(NULL);
            if (new_ms >= 0) {
                int delta = new_ms - c;
                if (delta <= 0) accept = 1;
                else if (((double)rand() / RAND_MAX) < exp(-(double)delta / T)) accept = 1;
            }
            if (accept) { for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i; }
            else { for (int i = 0; i < L; i++) orders[m][a + i] = seg[i]; for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i; }
        } else {
            int v = crit_ops[rand() % ncrit];
            int j = v / NM, k = v % NM; int m = route[j][k]; int p = orderpos[m][j];
            int t = rand() % len[m];
            if (t == p) t = (t + 1) % len[m];
            remove_at(m, j); insert_at(m, j, t);
            int new_ms = makespan(NULL);
            int acc2 = 0;
            if (new_ms >= 0) {
                int delta = new_ms - c;
                if (delta <= 0) acc2 = 1;
                else if (((double)rand() / RAND_MAX) < exp(-(double)delta / T)) acc2 = 1;
            }
            if (acc2) { /* keep */ }
            else { remove_at(m, j); insert_at(m, j, p); }
        }
        since++;
        if (since > 6000 || (rand() % 5000 == 0)) { kick(SA_KICK); since = 0; }
    }
    return makespan(NULL);
}

/* strong refinement: greedy descent then SA improvver */
static void refine(void) {
    descend_insert(300);
    descend_block(80);
    sa_improve(LS_ITER);
}

static void load_orders(int src[NM][NJ]) {
    for (int m = 0; m < NM; m++) { len[m] = NJ; for (int p = 0; p < NJ; p++) { orders[m][p] = src[m][p]; orderpos[m][orders[m][p]] = p; } }
}
static void save_orders(int dst[NM][NJ]) {
    for (int m = 0; m < NM; m++) for (int p = 0; p < NJ; p++) dst[m][p] = orders[m][p];
}

/* random valid schedule via Giffler-Thompson greedy with random dispatch */
static void random_schedule(void) {
    int nj[NJ]; for (int j = 0; j < NJ; j++) nj[j] = 0;
    int mready[NM]; for (int m = 0; m < NM; m++) mready[m] = 0;
    int jcompl[NJ]; for (int j = 0; j < NJ; j++) jcompl[j] = 0;
    for (int m = 0; m < NM; m++) len[m] = 0;
    int cand[NJ];
    for (int step = 0; step < NOPS; step++) {
        int nc = 0;
        for (int j = 0; j < NJ; j++) if (nj[j] < NM) cand[nc++] = j;
        int j = cand[rand() % nc];
        int m = route[j][nj[j]];
        orders[m][len[m]++] = j;
        int fin = (mready[m] > jcompl[j] ? mready[m] : jcompl[j]) + dur[j][nj[j]];
        mready[m] = fin; jcompl[j] = fin; nj[j]++;
    }
    for (int m = 0; m < NM; m++) for (int p = 0; p < NJ; p++) orderpos[m][orders[m][p]] = p;
}

/* G&T crossover: combine two parents into child (always acyclic).
   Each step we scan each parent's machine order for the FIRST job that is
   currently eligible (nj[j] == pos[j][m]); eligibility naturally disappears
   once a job's op on m is scheduled (nj[j] becomes >pos).  Because at least one
   job is always eligible, best_m is never -1 and no buffer is overrun. */
static void crossover(int p1[NM][NJ], int p2[NM][NJ], int child[NM][NJ]) {
    int nj[NJ]; for (int j = 0; j < NJ; j++) nj[j] = 0;
    int mready[NM]; for (int m = 0; m < NM; m++) mready[m] = 0;
    int jcompl[NJ]; for (int j = 0; j < NJ; j++) jcompl[j] = 0;
    int clen[NM]; for (int m = 0; m < NM; m++) clen[m] = 0;
    for (int step = 0; step < NOPS; step++) {
        int best_ct = INF, best_m = -1, best_j = -1;
        for (int m = 0; m < NM; m++) {
            int cand = -1, ct = INF;
            for (int p = 0; p < NJ; p++) { int j = p1[m][p]; if (nj[j] == pos[j][m]) { ct = (mready[m] > jcompl[j] ? mready[m] : jcompl[j]) + dur[j][pos[j][m]]; cand = j; break; } }
            if (cand >= 0 && ct < best_ct) { best_ct = ct; best_m = m; best_j = cand; }
            for (int p = 0; p < NJ; p++) { int j = p2[m][p]; if (nj[j] == pos[j][m]) { ct = (mready[m] > jcompl[j] ? mready[m] : jcompl[j]) + dur[j][pos[j][m]]; cand = j; break; } }
            if (cand >= 0 && ct < best_ct) { best_ct = ct; best_m = m; best_j = cand; }
        }
        int m = best_m, j = best_j;
        child[m][clen[m]++] = j;
        int fin = (mready[m] > jcompl[j] ? mready[m] : jcompl[j]) + dur[j][pos[j][m]];
        mready[m] = fin; jcompl[j] = fin; nj[j]++;
    }
    for (int m = 0; m < NM; m++) len[m] = clen[m];
}

#define MAXPOP 60
static int pop[MAXPOP][NM][NJ];
static int pop_ms[MAXPOP];
static int best_orders[NM][NJ];
static int best_ms = INF;

int main(int argc, char **argv) {
    int seconds = (argc > 1) ? atoi(argv[1]) : 1800;
    unsigned int seed = (argc > 2) ? (unsigned int)atoi(argv[2]) : (unsigned int)(time(0) ^ clock());
    const char *outpath = (argc > 3) ? argv[3] : "best_hga.txt";
    int POP = (argc > 4) ? atoi(argv[4]) : 30;
    if (POP < 2) POP = 2; if (POP > MAXPOP) POP = MAXPOP;
    if (argc > 5) LS_ITER = atoi(argv[5]) * 1000;  /* offspring SA budget (thousands) */
    srand(seed);

    FILE *f = fopen("instance.txt", "r");
    if (!f) { fprintf(stderr, "cannot open instance.txt\n"); return 1; }
    int nj, nm; fscanf(f, "%d %d", &nj, &nm);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) { int m, dd; fscanf(f, "%d %d", &m, &dd); route[j][k] = m; dur[j][k] = dd; }
    fclose(f);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) pos[j][route[j][k]] = k;

    FILE *g = fopen("baseline_orders.txt", "r");
    if (!g) { fprintf(stderr, "cannot open baseline_orders.txt\n"); return 1; }
    int baselines[NM][NJ];
    for (int m = 0; m < NM; m++) { for (int p = 0; p < NJ; p++) { fscanf(g, "%d", &baselines[m][p]); } }
    fclose(g);

    printf("HGA start: seconds=%d seed=%u POP=%d\n", seconds, seed, POP);

    /* initialize population with random schedules + local search */
    for (int i = 0; i < POP; i++) {
        random_schedule();
        local_search();
        save_orders(pop[i]);
        pop_ms[i] = makespan(NULL);
        if (pop_ms[i] < best_ms) { best_ms = pop_ms[i]; save_orders(best_orders); }
    }
    printf("init best = %d\n", best_ms);

    int iter = 0, last_report = 0;
    clock_t t0 = clock(); double elapsed = 0;
    while (elapsed < seconds) {
        iter++;
        /* tournament selection */
        int a = rand() % POP, b = rand() % POP;
        int p1 = (pop_ms[a] < pop_ms[b]) ? a : b;
        int c = rand() % POP, d = rand() % POP;
        int p2 = (pop_ms[c] < pop_ms[d]) ? c : d;
        if (p1 == p2) p2 = (p2 + 1) % POP;

        int child[NM][NJ];
        crossover(pop[p1], pop[p2], child);
        load_orders(child);
        refine();
        int c_ms = makespan(NULL);

        /* find worst */
        int w = 0; for (int i = 1; i < POP; i++) if (pop_ms[i] > pop_ms[w]) w = i;
        if (c_ms < pop_ms[w]) { save_orders(pop[w]); pop_ms[w] = c_ms; }
        if (c_ms < best_ms) {
            best_ms = c_ms; save_orders(best_orders);
            printf("  *** NEW BEST = %d at t=%.0fs gen=%d\n", best_ms, elapsed, iter); fflush(stdout);
        }
        /* occasional diversity injection */
        if (rand() % 300 == 0) {
            int r = rand() % POP;
            random_schedule(); local_search();
            int rms = makespan(NULL);
            if (rms < pop_ms[r]) { save_orders(pop[r]); pop_ms[r] = rms; }
        }

        elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
        int es = (int)elapsed;
        if (es > 0 && es % 30 == 0 && es != last_report) {
            last_report = es;
            printf("t=%ds gen=%d best=%d popworst=%d\n", es, iter, best_ms, pop_ms[w]);
            fflush(stdout);
        }
    }
    printf("generations = %d  best makespan = %d\n", iter, best_ms);
    FILE *o = fopen(outpath, "w");
    if (o) { for (int m = 0; m < NM; m++) { for (int p = 0; p < NJ; p++) fprintf(o, "%d%c", best_orders[m][p], p == NJ - 1 ? '\n' : ' '); } fclose(o); }
    return 0;
}
