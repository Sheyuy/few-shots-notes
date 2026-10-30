#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NJ 30
#define NM 20
#define NOPS (NJ*NM)  /* 600 */

static int route[NJ][NM];
static int dur[NJ][NM];
static int pos[NJ][NM];        /* pos[job][machine] = op index k */
static int orders[NM][NJ+4];     /* orders[machine][pos] = job */
static int orderpos[NM][NJ+4];   /* orderpos[machine][job] = pos */

static int jsucc[NOPS], msucc[NOPS], indeg[NOPS];
static int start[NOPS], remaining[NOPS], topo[NOPS];

/* returns makespan, fills critmask (1 if op on a critical path).
   returns -1 if the disjunctive graph has a cycle (invalid). */
static int makespan(int *critmask) {
    for (int v = 0; v < NOPS; v++) { jsucc[v] = -1; msucc[v] = -1; indeg[v] = 0; start[v] = 0; }
    for (int j = 0; j < NJ; j++) {
        for (int k = 0; k < NM - 1; k++) {
            int u = j * NM + k, w = j * NM + k + 1;
            jsucc[u] = w; indeg[w]++;
        }
    }
    for (int m = 0; m < NM; m++) {
        for (int p = 0; p < NJ - 1; p++) {
            int a = orders[m][p], b = orders[m][p + 1];
            int u = a * NM + pos[a][m], v = b * NM + pos[b][m];
            msucc[u] = v; indeg[v]++;
        }
    }
    int done = 0;
    int stack[NOPS]; int sp = 0;
    for (int v = 0; v < NOPS; v++) if (indeg[v] == 0) stack[sp++] = v;
    while (sp > 0) {
        int u = stack[--sp];
        topo[done++] = u;
        int fin = start[u] + dur[u / NM][u % NM];
        if (jsucc[u] >= 0) {
            if (fin > start[jsucc[u]]) start[jsucc[u]] = fin;
            if (--indeg[jsucc[u]] == 0) stack[sp++] = jsucc[u];
        }
        if (msucc[u] >= 0) {
            if (fin > start[msucc[u]]) start[msucc[u]] = fin;
            if (--indeg[msucc[u]] == 0) stack[sp++] = msucc[u];
        }
    }
    if (done != NOPS) return -1; /* cycle */
    int makesp = 0;
    for (int v = 0; v < NOPS; v++) {
        int fin = start[v] + dur[v / NM][v % NM];
        if (fin > makesp) makesp = fin;
    }
    for (int idx = NOPS - 1; idx >= 0; idx--) {
        int u = topo[idx];
        int r = dur[u / NM][u % NM];
        if (jsucc[u] >= 0) { int t = dur[u / NM][u % NM] + remaining[jsucc[u]]; if (t > r) r = t; }
        if (msucc[u] >= 0) { int t = dur[u / NM][u % NM] + remaining[msucc[u]]; if (t > r) r = t; }
        remaining[u] = r;
    }
    for (int v = 0; v < NOPS; v++) critmask[v] = (start[v] + remaining[v] == makesp) ? 1 : 0;
    return makesp;
}

/* remove job at position p on machine m, insert it at position t (0..NJ-1). */
static void do_insert(int m, int p, int t) {
    int j = orders[m][p];
    int tmp[NJ]; int idx = 0;
    for (int i = 0; i < NJ; i++) if (i != p) tmp[idx++] = orders[m][i];
    int k = 0;
    for (int i = 0; i < t; i++) orders[m][i] = tmp[k++];
    orders[m][t] = j;
    for (int i = t + 1; i < NJ; i++) orders[m][i] = tmp[k++];
    for (int i = 0; i < NJ; i++) orderpos[m][orders[m][i]] = i;
}

static int best_orders[NM][NJ+4];
static int best_ms = 1 << 30;

static void save_best(int ms) {
    if (ms >= 0 && ms < best_ms) {
        best_ms = ms;
        memcpy(best_orders, orders, sizeof(orders));
    }
}

int main(int argc, char **argv) {
    int seconds = (argc > 1) ? atoi(argv[1]) : 60;
    unsigned int seed = (argc > 2) ? (unsigned int)atoi(argv[2]) : (unsigned int)(time(0) ^ clock());
    int use_baseline = (argc > 3) ? atoi(argv[3]) : 1;
    const char *outpath = (argc > 4) ? argv[4] : "best_orders.txt";
    srand(seed);

    FILE *f = fopen("instance.txt", "r");
    if (!f) { fprintf(stderr, "cannot open instance.txt\n"); return 1; }
    int nj, nm;
    fscanf(f, "%d %d", &nj, &nm);
    for (int j = 0; j < NJ; j++)
        for (int k = 0; k < NM; k++) { int m, d; fscanf(f, "%d %d", &m, &d); route[j][k] = m; dur[j][k] = d; }
    fclose(f);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) pos[j][route[j][k]] = k;

    if (use_baseline) {
        FILE *g = fopen("baseline_orders.txt", "r");
        if (!g) { fprintf(stderr, "cannot open baseline_orders.txt\n"); return 1; }
        for (int m = 0; m < NM; m++) for (int p = 0; p < NJ; p++) { fscanf(g, "%d", &orders[m][p]); orderpos[m][orders[m][p]] = p; }
        fclose(g);
    } else {
        for (int m = 0; m < NM; m++) {
            for (int p = 0; p < NJ; p++) { orders[m][p] = p; orderpos[m][p] = p; }
            for (int i = NJ - 1; i > 0; i--) { int j = rand() % (i + 1); int t = orders[m][i]; orders[m][i] = orders[m][j]; orders[m][j] = t; orderpos[m][orders[m][i]] = i; orderpos[m][orders[m][j]] = j; }
        }
    }

    int crit[NOPS];
    int cur = makespan(crit);
    printf("initial makespan = %d\n", cur);
    save_best(cur);

    int tabu_pos[NM][NJ];
    int tabu_exp[NM][NJ];
    for (int m = 0; m < NM; m++) for (int j = 0; j < NJ; j++) tabu_exp[m][j] = -1;
    int tenure = 6 + rand() % 6;
    int iter = 0;
    int since_improve = 0;
    int last_report = 0;

    clock_t t0 = clock();
    double elapsed = 0;
    while (elapsed < seconds) {
        iter++;
        cur = makespan(crit);
        int crit_ops[NOPS]; int ncrit = 0;
        for (int v = 0; v < NOPS; v++) if (crit[v]) crit_ops[ncrit++] = v;

        int best_move_ms = 1 << 30;
        int best_m = -1, best_p = -1, best_t = -1, best_j = -1;

        for (int ci = 0; ci < ncrit; ci++) {
            int v = crit_ops[ci];
            int j = v / NM, k = v % NM;
            int m = route[j][k];
            int p = orderpos[m][j];
            for (int t = 0; t < NJ; t++) {
                if (t == p) continue;
                do_insert(m, p, t);
                int ms = makespan(crit);
                do_insert(m, t, p); /* revert */
                if (ms < 0) continue;
                int is_tabu = (iter < tabu_exp[m][j] && tabu_pos[m][j] == t) ? 1 : 0;
                int aspiration = (ms < best_ms) ? 1 : 0;
                if (is_tabu && !aspiration) continue;
                if (ms < best_move_ms) { best_move_ms = ms; best_m = m; best_p = p; best_t = t; best_j = j; }
            }
        }

        if (best_m >= 0) {
            do_insert(best_m, best_p, best_t);
            tabu_pos[best_m][best_j] = best_p;
            tabu_exp[best_m][best_j] = iter + tenure;
            if (best_move_ms < best_ms) { best_ms = best_move_ms; memcpy(best_orders, orders, sizeof(orders)); since_improve = 0; }
            else since_improve++;
        } else {
            int m = rand() % NM, p = rand() % (NJ - 1);
            do_insert(m, p, p + 1 > NJ - 1 ? p - 1 : p + 1);
            since_improve++;
        }

        if (since_improve > 4000) {
            memcpy(orders, best_orders, sizeof(orders));
            for (int m = 0; m < NM; m++)
                for (int q = 0; q < 12; q++) {
                    int p = rand() % (NJ - 1);
                    do_insert(m, p, p + 1 > NJ - 1 ? p - 1 : p + 1);
                    if (makespan(crit) < 0) do_insert(m, p + 1 > NJ - 1 ? p - 1 : p + 1, p);
                }
            for (int m = 0; m < NM; m++) for (int j = 0; j < NJ; j++) orderpos[m][orders[m][j]] = j;
            since_improve = 0;
        }
        elapsed = (double)(clock() - t0) / CLOCKS_PER_SEC;
        int es = (int)elapsed;
        if (es > 0 && es % 30 == 0 && es != last_report) {
            last_report = es;
            printf("t=%ds iter=%d best=%d\n", es, iter, best_ms);
            fflush(stdout);
        }
    }

    printf("iterations = %d  best makespan = %d\n", iter, best_ms);
    FILE *o = fopen(outpath, "w");
    if (o) {
        for (int m = 0; m < NM; m++) {
            for (int p = 0; p < NJ; p++) fprintf(o, "%d%c", best_orders[m][p], p == NJ - 1 ? '\n' : ' ');
        }
        fclose(o);
    }
    return 0;
}
