/* extsat.c -- exact extension test: can a valid 35-vertex (4,6)-graph B be
 * extended to a valid 36-vertex (4,6)-graph?
 *
 * For a new vertex v with neighbourhood S (over the 35 vertices of B):
 *
 *     B + v is a valid 36-vertex (4,6)-graph
 *        <=>  B[S] has no triangle              (else v + triangle = K4)
 *        <=>  B[V\S] has no independent 5-set    (else v + that set = I6)
 *
 * With x_i = "i in S":
 *
 *     for each triangle T={a,b,c} of B :  (~x_a | ~x_b | ~x_c)
 *     for each independent 5-set A of B : (x_a | x_b | x_c | x_d | x_e)
 *
 * 35 variables, ~1200 clauses: an exact decision, solved with minisat.
 * The Python version was dominated by enumerating the independent 5-sets, so
 * that part is done here with bitmask DFS.
 *
 * Input : text file, one graph per line, "n m0 m1 ..." (hex adjacency masks).
 * Output: one line per graph; "HIT" lines carry a 36-vertex certificate.
 *
 * Build: gcc -O2 -o extsat extsat.c -lm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAXN 40
#define MAXI5 8192
typedef uint64_t mask;

static int  gn;
static mask gadj[MAXN];

/* triangles: list of vertex triples */
static int tri_list[MAXN * 32][3];
static int tri_total;

/* independent 5-sets: list of vertex 5-tuples */
static int ind5_list[MAXI5][5];
static int ind5_total;

/* per-vertex incidence */
static int tri_of[MAXN][512], ntri_of[MAXN];
static int ind5_of[MAXN][MAXI5], nind5_of[MAXN];

static mask gnonadj[MAXN];

static void rec_ind5(mask cand, int depth, int *chosen)
{
    if (depth == 5) {
        int *t = ind5_list[ind5_total++];
        for (int a = 0; a < 5; a++) t[a] = chosen[a];
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
    tri_total = 0;
    ind5_total = 0;
    memset(ntri_of, 0, sizeof(ntri_of));
    memset(nind5_of, 0, sizeof(nind5_of));

    for (j = 0; j < n; j++) {
        mask gt = ~((((mask)1 << (j + 1)) - 1)) & full;
        for (i = 0; i < j; i++) {
            if (!((gadj[i] >> j) & 1)) continue;
            mask c = gadj[i] & gadj[j] & gt;
            while (c) {
                mask lo = c & (~c + 1);
                k = __builtin_ctzll(lo);
                c ^= lo;
                int *t = tri_list[tri_total++];
                t[0] = i; t[1] = j; t[2] = k;
            }
        }
    }
    for (int x = 0; x < tri_total; x++)
        for (int a = 0; a < 3; a++)
            tri_of[tri_list[x][a]][ntri_of[tri_list[x][a]]++] = x;

    for (i = 0; i < n; i++)
        gnonadj[i] = full & ~gadj[i] & ~((mask)1 << i);
    int chosen[8];
    rec_ind5(full, 0, chosen);
    for (int x = 0; x < ind5_total; x++)
        for (int a = 0; a < 5; a++)
            ind5_of[ind5_list[x][a]][nind5_of[ind5_list[x][a]]++] = x;
}

/* ---- independent verification of an n-vertex graph given as bitmask rows */
static int verify_k4(mask *H, int n)
{
    mask full = ((mask)1 << n) - 1;
    int k4 = 0;
    for (int j = 0; j < n; j++) {
        mask gt = ~((((mask)1 << (j + 1)) - 1)) & full;
        for (int i = 0; i < j; i++) {
            if (!((H[i] >> j) & 1)) continue;
            mask c = H[i] & H[j] & gt;
            while (c) {
                mask lo = c & (~c + 1);
                int v = __builtin_ctzll(lo);
                c ^= lo;
                int cnt = 0;
                mask m = c & H[v];
                while (m) { mask l2 = m & (~m + 1); m ^= l2; cnt++; }
                k4 += cnt;
            }
        }
    }
    return k4;
}

static long verify_i6(mask *H, int n)
{
    mask full = ((mask)1 << n) - 1;
    mask nonadj[MAXN];
    for (int i = 0; i < n; i++) nonadj[i] = full & ~H[i] & ~((mask)1 << i);
    long cnt = 0;
    int chosen[8];
    (void)chosen;
    /* recursive helper via explicit stack-free recursion using a nested fn */
    void rec(mask cand, int depth)
    {
        if (depth == 6) { cnt++; return; }
        while (cand) {
            mask lo = cand & (~cand + 1);
            int v = __builtin_ctzll(lo);
            cand ^= lo;
            rec(cand & nonadj[v], depth + 1);
        }
    }
    rec(full, 0);
    return cnt;
}

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: extsat <graphs.txt> <solver> [outdir]\n"); return 1; }
    const char *solver = argv[2];
    const char *outdir = (argc > 3) ? argv[3] : ".";
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror(argv[1]); return 1; }

    char cnf[512], sat[512], cmd[1024];
    snprintf(cnf, sizeof(cnf), "%s/extsat.cnf", outdir);
    snprintf(sat, sizeof(sat), "%s/extsat.sat", outdir);

    long gidx = 0, hits = 0, unsat = 0, errs = 0;
    int n;
    while (fscanf(fp, "%d", &n) == 1) {
        mask rows[MAXN];
        for (int i = 0; i < n; i++) {
            unsigned long long h;
            if (fscanf(fp, "%llx", &h) != 1) { fprintf(stderr, "short read\n"); return 1; }
            rows[i] = (mask)h;
        }
        for (int i = 0; i < n; i++) gadj[i] = rows[i];
        gn = n;
        build_index();

        /* ---- CNF ---- */
        FILE *cf = fopen(cnf, "w");
        fprintf(cf, "p cnf %d %d\n", n, tri_total + ind5_total);
        for (int x = 0; x < tri_total; x++)
            fprintf(cf, "-%d -%d -%d 0\n", tri_list[x][0] + 1, tri_list[x][1] + 1,
                    tri_list[x][2] + 1);
        for (int x = 0; x < ind5_total; x++) {
            const int *t = ind5_list[x];
            fprintf(cf, "%d %d %d %d %d 0\n", t[0] + 1, t[1] + 1, t[2] + 1, t[3] + 1,
                    t[4] + 1);
        }
        fclose(cf);

        snprintf(cmd, sizeof(cmd), "%s %s %s > /dev/null 2>&1", solver, cnf, sat);
        int rc = system(cmd);

        /* ---- parse model ---- */
        FILE *sf = fopen(sat, "r");
        if (!sf) { errs++; gidx++; continue; }
        int is_unsat = 0, model_ok = 0;
        int xv[64];
        memset(xv, 0, sizeof(xv));
        char line[4096];
        while (fgets(line, sizeof(line), sf)) {
            if (strncmp(line, "UNSAT", 5) == 0) { is_unsat = 1; break; }
            char *p = line;
            while (*p) {
                while (*p == ' ' || *p == '\t') p++;
                if (!*p) break;
                if (*p == 'v' || *p == 'V') { p++; continue; }
                if (*p == 's' || *p == 'S') { while (*p && *p != '\n') p++; continue; }
                if (*p == '-') {
                    p++;
                    int v = 0;
                    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
                    if (v >= 1 && v <= n) xv[v] = 0;
                    continue;
                }
                if (*p >= '0' && *p <= '9') {
                    int v = 0;
                    while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); p++; }
                    if (v >= 1 && v <= n) xv[v] = 1;
                    continue;
                }
                p++;
            }
        }
        fclose(sf);
        if (rc != 0 && !is_unsat) { errs++; gidx++; continue; }
        if (is_unsat) {
            unsat++;
            if ((gidx + 1) % 200 == 0)
                printf("  ... %ld  unsat=%ld hits=%ld\n", gidx + 1, unsat, hits);
            gidx++;
            continue;
        }
        model_ok = 1;
        (void)model_ok;

        /* ---- build the 36-vertex graph and verify independently ---- */
        mask H[MAXN + 1];
        int N = n + 1;
        for (int i = 0; i < n; i++) H[i] = gadj[i] & (((mask)1 << n) - 1);
        H[n] = 0;
        for (int i = 0; i < n; i++)
            if (xv[i + 1]) { H[i] |= ((mask)1 << n); H[n] |= ((mask)1 << i); }

        int k4 = verify_k4(H, N);
        long i6 = verify_i6(H, N);
        if (k4 == 0 && i6 == 0) {
            hits++;
            printf("HIT g%ld: valid 36-vertex graph!\n", gidx);
            char fn[512];
            snprintf(fn, sizeof(fn), "%s/cert36_g%ld.json", outdir, gidx);
            FILE *of = fopen(fn, "w");
            fprintf(of, "{\"adjacency\": [");
            for (int i = 0; i < N; i++) {
                fprintf(of, "%s\"", i ? ", " : "");
                for (int j = 0; j < N; j++) fputc((H[i] >> j) & 1 ? '1' : '0', of);
            }
            fprintf(of, "]}\n");
            fclose(of);
            printf("     -> %s\n", fn);
        } else {
            printf("BAD g%ld: solver said SAT but K4=%d I6=%ld\n", gidx, k4, i6);
        }
        gidx++;
    }
    fclose(fp);
    printf("--- graphs=%ld  UNSAT=%ld  HIT=%ld  errors=%ld\n", gidx, unsat, hits, errs);
    return 0;
}
