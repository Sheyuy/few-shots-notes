#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NJ 30
#define NM 20
#define NOPS (NJ*NM)

static int route[NJ][NM];
static int dur[NJ][NM];
static int pos[NJ][NM];
static int orders[NM][NJ];
static int orderpos[NM][NJ];
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

int main(int argc, char **argv) {
    const char *ordpath = (argc > 1) ? argv[1] : "baseline_orders.txt";
    FILE *f = fopen("instance.txt", "r");
    if (!f) { fprintf(stderr, "cannot open instance.txt\n"); return 1; }
    int nj, nm; fscanf(f, "%d %d", &nj, &nm);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) { int m, dd; fscanf(f, "%d %d", &m, &dd); route[j][k] = m; dur[j][k] = dd; }
    fclose(f);
    for (int j = 0; j < NJ; j++) for (int k = 0; k < NM; k++) pos[j][route[j][k]] = k;

    FILE *g = fopen(ordpath, "r");
    if (!g) { fprintf(stderr, "cannot open %s\n", ordpath); return 1; }
    for (int m = 0; m < NM; m++) { len[m] = NJ; for (int p = 0; p < NJ; p++) { fscanf(g, "%d", &orders[m][p]); orderpos[m][orders[m][p]] = p; } }
    fclose(g);

    int base = makespan(NULL);
    printf("base makespan = %d\n", base);

    /* exhaustive single-insertion neighborhood over ALL jobs on every machine */
    int global_min = base;
    int improving = 0, nimproving = 0;
    int worst = base;
    for (int m = 0; m < NM; m++) {
        for (int p = 0; p < len[m]; p++) {
            int j = orders[m][p];
            for (int t = 0; t < len[m]; t++) {
                if (t == p) continue;
                remove_at(m, j); insert_at(m, j, t);
                int ms = makespan(NULL);
                if (ms >= 0) {
                    if (ms < global_min) global_min = ms;
                    if (ms < base) { improving = 1; nimproving++; }
                    if (ms > worst) worst = ms;
                }
                remove_at(m, j); insert_at(m, j, p); /* restore */
            }
        }
    }
    printf("exhaustive single-insertion neighborhood:\n");
    printf("  min makespan among all single-insertion neighbors = %d\n", global_min);
    printf("  max makespan among all single-insertion neighbors = %d\n", worst);
    printf("  number of strictly improving single-insertion moves = %d\n", nimproving);
    printf("  => %s\n", (global_min < base) ? "NOT a local optimum (improving move exists)" : "LOCAL OPTIMUM under single-insertion neighborhood");
    return 0;
}
