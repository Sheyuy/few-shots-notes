/* sat36.c -- CNF encoder for "exists a 36-vertex (4,6)-Ramsey graph".
 *
 * A valid graph H on 36 vertices has no K4 and no independent 6-set.
 *
 * Variables: x_{ij} for 1 <= i < j <= 36   (630 vars, 1-based DIMACS ids)
 *
 * Clauses:
 *   for every 4-subset S :  (~x_ij for all 6 edges in S)      -> S is not a K4
 *   for every 6-subset S :  ( x_ij for all 15 edges in S)     -> S is not independent
 *
 * Extra (all of them are *necessary* conditions, not heuristics):
 *
 *  (1) DEGREE RANGE 11..17.  For any vertex v of a valid 36-vertex graph:
 *        deg(v) <= R(3,6)-1 = 17   (N(v) is triangle-free and has no I6)
 *        deg(v) >= 35-(R(4,5)-1) = 11   (the non-neighbourhood has no K4 and no I5)
 *      A uniform random graph on 36 vertices has average degree 17.5, so this
 *      constraint alone throws away roughly half of the space.  Encoded with a
 *      Sinz sequential counter (O(m*k) clauses) instead of a naive expansion.
 *
 *  (2) SYMMETRY BREAKING.  WLOG pick a vertex of degree d, relabel it 0, and
 *      relabel its neighbours 1..d and its non-neighbours d+1..35.  Run one
 *      instance per d in 11..17; the union of the seven instances contains
 *      every solution.
 *
 * Build: gcc -O2 -o sat36 sat36.c
 * Usage: sat36 <out.cnf> [mode] [d]
 *        mode 0 = plain, 1 = + degree range, 2 = + degree range + symmetry break
 *        d = the fixed degree of vertex 0 (used when mode = 2)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int N = 36;
static int DEGLO = 11, DEGHI = 17;
#define E 630                    /* C(36,2) */

static int eid[40][40];            /* DIMACS var id of the edge (i,j), i<j */
static int nvars;

static int evar(int i, int j)
{
    if (i > j) { int t = i; i = j; j = t; }
    return eid[i][j];
}

/* ---------------- clause emission ---------------- */
static FILE *out;
static long nclauses;

static void emit(int n, const int *lits)
{
    for (int i = 0; i < n; i++) fprintf(out, "%d ", lits[i]);
    fputs("0\n", out);
    nclauses++;
}

/* Sinz sequential counter: at most k of the given literals may be true.
 * lits[] holds DIMACS ids (signed).  Returns the number of new variables. */
static int at_most_k(const int *lits, int m, int k)
{
    if (k >= m) return 0;
    /* s[i][j] : at least j of lits[0..i-1] are true */
    int **s = malloc(sizeof(int *) * (m + 1));
    for (int i = 0; i <= m; i++) s[i] = calloc(k + 2, sizeof(int));
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= k; j++) s[i][j] = ++nvars;

    int buf[8];
    for (int i = 1; i <= m; i++) {
        int l = lits[i - 1];
        /* (~l | s[i][1]) */
        buf[0] = -l; buf[1] = s[i][1]; emit(2, buf);
        if (i > 1) {
            /* (~s[i-1][1] | s[i][1]) */
            buf[0] = -s[i - 1][1]; buf[1] = s[i][1]; emit(2, buf);
        } else {
            /* s[1][1] -> l  :  (~s[1][1] | l) */
            buf[0] = -s[i][1]; buf[1] = l; emit(2, buf);
        }
        for (int j = 2; j <= k; j++) {
            /* (~l | ~s[i-1][j-1] | s[i][j]) */
            if (i > 1) {
                buf[0] = -l; buf[1] = -s[i - 1][j - 1]; buf[2] = s[i][j];
                emit(3, buf);
            } else {
                /* s[1][j] is false for j>=2 */
                buf[0] = -l; buf[1] = -s[i][j]; emit(2, buf);
            }
            /* (~l | ~s[i-1][j]) */
            if (i > 1) {
                buf[0] = -l; buf[1] = -s[i - 1][j]; emit(2, buf);
            } else {
                buf[0] = -l; buf[1] = -s[i][j]; emit(2, buf);
            }
        }
    }
    /* (~s[m][k]) */
    buf[0] = -s[m][k]; emit(1, buf);

    for (int i = 0; i <= m; i++) free(s[i]);
    free(s);
    return m * k;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: sat36 <out.cnf> [mode] [d]\n"); return 1; }
    int mode = (argc > 2) ? atoi(argv[2]) : 0;
    int dfix = (argc > 3) ? atoi(argv[3]) : -1;
    if (getenv("NVERT")) N = atoi(getenv("NVERT"));
    if (getenv("DEGLO")) DEGLO = atoi(getenv("DEGLO"));
    if (getenv("DEGHI")) DEGHI = atoi(getenv("DEGHI"));
    const char *pinf = getenv("PIN");
    if (mode == 2 && (dfix < DEGLO || dfix > DEGHI)) {
        fprintf(stderr, "mode 2 needs d in DEGLO..DEGHI\n"); return 1;
    }

    /* assign edge variable ids */
    nvars = 0;
    for (int i = 0; i < N; i++)
        for (int j = i + 1; j < N; j++) eid[i][j] = ++nvars;
    int base_vars = nvars;

    /* count clauses first (we need the header) */
    long c_k4 = 0, c_i6 = 0;
    /* C(36,4) and C(36,6) */
    {
        long a = 1;
        for (int i = 0; i < 4; i++) a = a * (N - i) / (i + 1);
        c_k4 = a;
        a = 1;
        for (int i = 0; i < 6; i++) a = a * (N - i) / (i + 1);
        c_i6 = a;
    }
    /* degree constraints: per vertex, at most 17 incident edges true, and at
     * most 24 of them false.  Sinz costs about 3*m*k clauses each. */
    long deg_clauses = 0;
    int use_deg = (mode >= 1) && getenv("USEDEG");
    if (use_deg) {
        int m = N - 1, k1 = DEGHI, k2 = (N-1) - DEGLO;
        deg_clauses = (long)N * (3L * m * k1 + 3L * m * k2 + 4L * m + 4L);
    }
    long unit_extra = (mode == 2) ? (N - 1) : 0;
    long total = c_k4 + c_i6 + deg_clauses + unit_extra;

    FILE *real = fopen(argv[1], "w");
    if (!real) { perror(argv[1]); return 1; }

    /* the header needs the FINAL variable count, which depends on the degree
     * counters; so build the body into a temp file and then prepend. */
    char tmpname[512];
    snprintf(tmpname, sizeof(tmpname), "%s.body", argv[1]);
    out = fopen(tmpname, "w");
    if (!out) { perror(tmpname); return 1; }
    FILE *body = out;

    /* --- K4 clauses: for each 4-subset, ~x over its 6 edges --- */
    int q[64];
    {
        int idx[4];
        for (idx[0] = 0; idx[0] < N; idx[0]++)
        for (idx[1] = idx[0] + 1; idx[1] < N; idx[1]++)
        for (idx[2] = idx[1] + 1; idx[2] < N; idx[2]++)
        for (idx[3] = idx[2] + 1; idx[3] < N; idx[3]++) {
            int n = 0;
            for (int a = 0; a < 4; a++)
                for (int b = a + 1; b < 4; b++)
                    q[n++] = -evar(idx[a], idx[b]);
            emit(6, q);
        }
    }
    /* --- I6 clauses: for each 6-subset, x over its 15 edges --- */
    {
        int idx[6];
        for (idx[0] = 0; idx[0] < N; idx[0]++)
        for (idx[1] = idx[0] + 1; idx[1] < N; idx[1]++)
        for (idx[2] = idx[1] + 1; idx[2] < N; idx[2]++)
        for (idx[3] = idx[2] + 1; idx[3] < N; idx[3]++)
        for (idx[4] = idx[3] + 1; idx[4] < N; idx[4]++)
        for (idx[5] = idx[4] + 1; idx[5] < N; idx[5]++) {
            int n = 0;
            for (int a = 0; a < 6; a++)
                for (int b = a + 1; b < 6; b++)
                    q[n++] = evar(idx[a], idx[b]);
            emit(15, q);
        }
    }
    /* --- degree constraints --- */
    if (use_deg) {
        for (int v = 0; v < N; v++) {
            int lits[N - 1], neg[N - 1], n = 0;
            for (int u = 0; u < N; u++) {
                if (u == v) continue;
                lits[n] = evar(v, u);
                neg[n] = -lits[n];
                n++;
            }
            at_most_k(lits, n, DEGHI);
            at_most_k(neg, n, (N-1) - DEGLO);
        }
    }
    /* --- PIN: force the edge variables to a given adjacency matrix --- */
    if (pinf) {
        FILE *pf = fopen(pinf, "r");
        if (!pf) { perror(pinf); return 1; }
        char *row = malloc(N + 8);
        for (int i = 0; i < N; i++) {
            if (!fgets(row, N + 8, pf)) { fprintf(stderr, "PIN: short file\n"); return 1; }
            for (int j = 0; j < N; j++) {
                if (i == j) continue;
                if (row[j] != '0' && row[j] != '1') continue;
                int v = evar(i, j);
                q[0] = (row[j] == '1') ? v : -v;
                emit(1, q);
            }
        }
        fclose(pf); free(row);
    }
    /* --- symmetry breaking: N(0) = {1..d} --- */
    if (mode == 2) {
        for (int u = 1; u < N; u++) {
            int lit = (u <= dfix) ? evar(0, u) : -evar(0, u);
            q[0] = lit; emit(1, q);
        }
    }
    fclose(body);

    /* now write the real file: header + body */
    fprintf(real, "p cnf %d %ld\n", nvars, nclauses);
    {
        FILE *b = fopen(tmpname, "r");
        char buf[1 << 16];
        size_t L;
        while ((L = fread(buf, 1, sizeof(buf), b)) > 0) fwrite(buf, 1, L, real);
        fclose(b);
        remove(tmpname);
    }
    fclose(real);
    fprintf(stderr, "vars=%d clauses=%ld (k4=%ld i6=%ld deg=%ld) mode=%d d=%d\n",
            nvars, nclauses, c_k4, c_i6, deg_clauses, mode, dfix);
    return 0;
}
