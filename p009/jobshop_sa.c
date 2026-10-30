#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define NJ 30
#define NM 20
#define NOPS (NJ*NM)

static int route[NJ][NM];
static int dur[NJ][NM];
static int pos[NJ][NM];
static int orders[NM][NJ+4];
static int orderpos[NM][NJ+4];
static int len[NM];

static int jsucc[NOPS], msucc[NOPS], indeg[NOPS];
static int start[NOPS], remaining[NOPS], topo[NOPS];

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
/* greedy reinsertion of one job: for each machine in route order, place its op at best position */
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

static int best_orders[NM][NJ+4];
static int best_ms = 1 << 30;

static int crit[NOPS];
static int cur_ms;

/* IG-style kick: remove d random jobs, reinsert greedily, then light descent. Returns new makespan. */
static int kick(int d) {
    int removed[NJ];
    for (int i = 0; i < d; i++) {
        int j;
        do { j = rand() % NJ; } while (i > 0 && (j == removed[0] || (i > 1 && j == removed[1]) || (i > 2 && j == removed[2]) || (i > 3 && j == removed[3]) || (i > 4 && j == removed[4])));
        removed[i] = j;
    }
    for (int i = 0; i < d; i++) for (int m = 0; m < NM; m++) if (orderpos[m][removed[i]] < len[m] && orders[m][orderpos[m][removed[i]]] == removed[i]) remove_at(m, removed[i]);
    for (int i = 0; i < d; i++) insert_job(removed[i]);
    /* light descent */
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

int main(int argc, char **argv) {
    int seconds = (argc > 1) ? atoi(argv[1]) : 1200;
    unsigned int seed = (argc > 2) ? (unsigned int)atoi(argv[2]) : (unsigned int)(time(0) ^ clock());
    const char *outpath = (argc > 3) ? argv[3] : "best_sa.txt";
    int kick_d = (argc > 4) ? atoi(argv[4]) : 5;
    double T0 = (argc > 5) ? atof(argv[5]) : 150.0;
    double Tend = (argc > 6) ? atof(argv[6]) : 0.05;
    if (kick_d <= 0) kick_d = 5;
    if (T0 <= 0.0) T0 = 150.0;
    if (Tend <= 0.0) Tend = 0.05;
    srand(seed);

    FILE *f = fopen("instance.txt", "r");
    if (!f) { fprintf(stderr, "cannot open instance.txt\n"); return 1; }
    int nj, nm; fscanf(f, "%d %d", &nj, &nm);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) { int m, dd; fscanf(f, "%d %d", &m, &dd); route[j][k] = m; dur[j][k] = dd; }
    fclose(f);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) pos[j][route[j][k]] = k;

    FILE *g = fopen("baseline_orders.txt", "r");
    if (!g) { fprintf(stderr, "cannot open baseline_orders.txt\n"); return 1; }
    for (int m = 0; m < NM; m++) { len[m] = NJ; for (int p = 0; p < NJ; p++) { fscanf(g, "%d", &orders[m][p]); orderpos[m][orders[m][p]] = p; } }
    fclose(g);

    cur_ms = makespan(crit);
    printf("initial makespan = %d (seed=%u kick_d=%d T0=%.1f Tend=%.3f)\n", cur_ms, seed, kick_d, T0, Tend);
    if (cur_ms >= 0 && cur_ms < best_ms) { best_ms = cur_ms; memcpy(best_orders, orders, sizeof(orders)); }

    int iter = 0, last_report = 0, since_improve = 0;
    clock_t t0 = clock(); double elapsed = 0;
    int accepted_up = 0;
    while (elapsed < seconds) {
        iter++;
        double frac = elapsed / seconds;
        double T = T0 * pow(Tend / T0, frac);  /* geometric cooling */
        int c = makespan(crit);
        if (c < 0) { /* current became cyclic (shouldn't happen); recover via kick */
            kick(kick_d); cur_ms = makespan(NULL); continue;
        }
        /* collect critical ops */
        int crit_ops[NOPS]; int ncrit = 0;
        for (int v = 0; v < NOPS; v++) if (crit[v]) crit_ops[ncrit++] = v;
        if (ncrit == 0) { /* not critical structure; just kick */
            cur_ms = kick(kick_d); since_improve = 0; continue;
        }
        /* find maximal consecutive critical blocks on each machine (length >= 2) */
        int bmachine[NM*NJ], bstart[NM*NJ], bend[NM*NJ], nblocks = 0;
        for (int m = 0; m < NM; m++) {
            int p = 0;
            while (p < len[m]) {
                int jp = orders[m][p];
                if (crit[jp * NM + pos[jp][m]]) {
                    int a = p;
                    while (p < len[m]) {
                        int jq = orders[m][p];
                        if (!crit[jq * NM + pos[jq][m]]) break;
                        p++;
                    }
                    int b = p - 1;
                    if (b - a >= 1) { bmachine[nblocks] = m; bstart[nblocks] = a; bend[nblocks] = b; nblocks++; }
                } else p++;
            }
        }

        int accept = 0;
        int use_block = (nblocks > 0 && (rand() % 100) < 50);
        if (use_block) {
            /* Nowicki-Smutnicki N5 (rotate a critical block) / N6 (reverse it) */
            int bi = rand() % nblocks;
            int m = bmachine[bi], a = bstart[bi], b = bend[bi], L = b - a + 1;
            int seg[NJ];
            for (int i = 0; i < L; i++) seg[i] = orders[m][a + i];
            int type = rand() % 3;  /* 0: rotate-left, 1: rotate-right, 2: reverse (N6) */
            for (int i = 0; i < L; i++) {
                int src;
                if (type == 0) src = seg[(i + 1) % L];
                else if (type == 1) src = seg[(i + L - 1) % L];
                else src = seg[L - 1 - i];
                orders[m][a + i] = src;
            }
            int new_ms = makespan(NULL);
            if (new_ms >= 0) {
                int delta = new_ms - c;
                if (delta <= 0) accept = 1;
                else if (((double)rand() / RAND_MAX) < exp(-(double)delta / T)) { accept = 1; accepted_up++; }
            }
            if (accept) {
                cur_ms = new_ms;
                for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i;  /* keep orderpos consistent */
                if (new_ms >= 0 && new_ms < best_ms) {
                    best_ms = new_ms; memcpy(best_orders, orders, sizeof(orders)); since_improve = 0;
                    printf("  *** NEW BEST = %d at t=%.0fs iter=%d (block type %d)\n", best_ms, elapsed, iter, type); fflush(stdout);
                }
            } else {
                for (int i = 0; i < L; i++) orders[m][a + i] = seg[i];  /* restore */
                for (int i = 0; i < len[m]; i++) orderpos[m][orders[m][i]] = i;
            }
        } else {
            /* single critical-op insertion move */
            int v = crit_ops[rand() % ncrit];
            int j = v / NM, k = v % NM; int m = route[j][k]; int p = orderpos[m][j];
            int t = rand() % len[m];
            if (t == p) t = (t + 1) % len[m];
            remove_at(m, j); insert_at(m, j, t);
            int new_ms = makespan(NULL);
            if (new_ms >= 0) {
                int delta = new_ms - c;
                if (delta <= 0) accept = 1;
                else if (((double)rand() / RAND_MAX) < exp(-(double)delta / T)) { accept = 1; accepted_up++; }
            }
            if (accept) {
                cur_ms = new_ms;
                if (new_ms >= 0 && new_ms < best_ms) {
                    best_ms = new_ms; memcpy(best_orders, orders, sizeof(orders)); since_improve = 0;
                    printf("  *** NEW BEST = %d at t=%.0fs iter=%d\n", best_ms, elapsed, iter); fflush(stdout);
                }
            } else {
                /* revert */
                remove_at(m, j); insert_at(m, j, p);
            }
        }
        /* plateau / kick escape */
        since_improve++;
        if (since_improve > 6000 || (rand() % 5000 == 0)) {
            cur_ms = kick(kick_d);
            since_improve = 0;
        }
        elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
        int es = (int)elapsed;
        if (es > 0 && es % 30 == 0 && es != last_report) {
            last_report = es;
            printf("t=%ds iter=%d cur=%d best=%d up=%d\n", es, iter, cur_ms, best_ms, accepted_up);
            fflush(stdout);
        }
    }
    printf("iterations = %d  best makespan = %d  uphill_accepted = %d\n", iter, best_ms, accepted_up);
    FILE *o = fopen(outpath, "w");
    if (o) { for (int m = 0; m < NM; m++) { for (int p = 0; p < NJ; p++) fprintf(o, "%d%c", best_orders[m][p], p == NJ - 1 ? '\n' : ' '); } fclose(o); }
    return 0;
}
