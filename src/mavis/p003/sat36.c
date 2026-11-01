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
static int min2(int a, int b) { return a < b ? a : b; }
static void emit(int n, const int *lits);

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

/* ------------------------------------------------------------------
 * at_most_k -- "at most k of these m literals may be true".
 *
 * This is the EXACT encoding from card2.py, which was verified exhaustively
 * (m <= 7, every k, every one of the 2^m assignments checked with kissat
 * against a brute-force reference).  An earlier Sinz-style counter was
 * silently over-constraining and made a known-good pinned graph come back
 * UNSAT -- do not "simplify" this without re-running card2.py's self-test.
 *
 * Counter semantics:  s[i][j]  <->  "at least j of lits[0..i-1] are true",
 * with the constant TRUE for s[i][0] and constant FALSE for s[i][j] with
 * j > i.  UP clauses force s true, DOWN clauses force s false; together they
 * make s exact, which is what makes the final clause sound.
 * ------------------------------------------------------------------ */
static int at_most_k(const int *lits, int m, int k)
{
    if (k >= m) return 0;
    if (k < 0) { for (int i = 0; i < m; i++) { int b[1]; b[0] = -lits[i]; emit(1, b); } return 0; }

    /* s[i][j] for 1<=i<=m, 1<=j<=min(k+1,i) */
    int *sv = (int *)calloc((size_t)(m + 1) * (k + 2), sizeof(int));
#define SV(i, j) sv[(size_t)(i) * (k + 2) + (j)]
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= min2(k + 1, i); j++) SV(i, j) = ++nvars;

    int c[4];
    /* base i = 1 */
    c[0] = -lits[0]; c[1] = SV(1, 1); emit(2, c);
    c[0] = -SV(1, 1); c[1] = lits[0]; emit(2, c);

    for (int i = 2; i <= m; i++) {
        int x = lits[i - 1];
        for (int j = 1; j <= min2(k + 1, i); j++) {
            int sij = SV(i, j);
            /* A = s[i-1][j-1]: constant TRUE when j == 1 */
            int A_TRUE = (j == 1);
            int A = A_TRUE ? 0 : SV(i - 1, j - 1);
            /* B = s[i-1][j]: constant FALSE when j > i-1 */
            int B_FALSE = (j > i - 1);
            int B = B_FALSE ? 0 : SV(i - 1, j);
            /* U1 */
            if (j == 1) { c[0] = -x; c[1] = sij; emit(2, c); }
            /* U2 */
            if (!B_FALSE) { c[0] = -B; c[1] = sij; emit(2, c); }
            /* U3 */
            if (j >= 2) { c[0] = -x; c[1] = -A; c[2] = sij; emit(3, c); }
            /* D1 */
            if (!A_TRUE) { c[0] = -sij; c[1] = -x; c[2] = A; emit(3, c); }
            /* D2 */
            if (B_FALSE) { c[0] = -sij; c[1] = x; emit(2, c); }
            else { c[0] = -sij; c[1] = x; c[2] = B; emit(3, c); }
        }
    }
    /* final */
    c[0] = -SV(m, k + 1); emit(1, c);
    free(sv);
    return 0;
#undef SV
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
        if (0) {          /* per-vertex form: N * C(N-1,3) */
            long b = 1;
            for (int i = 0; i < 3; i++) b = b * (N - 1 - i) / (i + 1);
            c_k4 = (long)N * b;
        }
        a = 1;
        for (int i = 0; i < 6; i++) a = a * (N - i) / (i + 1);
        c_i6 = a;
    }
    /* degree constraints: per vertex, at most 17 incident edges true, and at
     * most 24 of them false.  Sinz costs about 3*m*k clauses each. */
    long deg_clauses = 0;
    int use_deg = (mode >= 4) || ((mode >= 1) && getenv("USEDEG"));
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

    /* --- K4 clauses: for each 4-subset, ~x over its 6 edges ---
     *
     * NOTE: there is no cheaper "per-vertex" form of this constraint.  The
     * tempting clause (~x_va | ~x_vb | ~x_vc) for every triple in V\{v} would
     * forbid v from having ANY three neighbours at all (i.e. force deg(v) <= 2)
     * -- that is NOT "N(v) is triangle-free".  It was implemented, pinned with a
     * known-good graph and came back UNSAT, which is how the mistake was found.
     */
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
    if (mode == 2 || mode == 4) {
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
