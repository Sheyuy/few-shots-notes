/*
 * sortsat.c -- SAT encoding of
 *      "there is a sorting network on n channels with at most maxC
 *       comparators and depth at most d"
 *
 * Encoding (standard; cf. Bundala & Zavodny 2014, Codish et al. 2015):
 *   g[i][j][k]  comparator (i,j) sits in layer k           (0-based channels)
 *   u[i][k]     channel i is touched by some comparator in layer k
 *   v[m][i][k]  value on channel i after layer k for input m
 *   once        two comparators in one layer may not share a channel
 *   update      v[m][i][k] is min/max of the two incoming values
 *   sorts       v[m][0][d] <= v[m][1][d] <= ... <= v[m][n-1][d]
 *   card        total number of comparators <= maxC (Sinz sequential counter)
 *
 * The zero-one principle lets us check only the 0/1 inputs listed in
 * inputs.txt; the driver adds counterexamples until the instance is
 * satisfied for every one of the 2^n inputs (the "iterative encoding"
 * of Codish et al.).
 *
 * Usage:
 *   sortsat <n> <d> <maxC> <prefix.txt> <inputs.txt> <out.cnf>
 *     prefix.txt  one line per layer: "i j i j ..." (0-based); empty line = no
 *                 comparators in that layer; a line starting with '#' means
 *                 "no prefix given" (all layers free)
 *     inputs.txt  one integer mask per line (bit i = channel i), the 0/1
 *                 inputs the network must sort
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *out;
static long nvars = 0;

/* clause buffer ------------------------------------------------------------ */
static int **cbuf = NULL;
static int *clen = NULL;
static long nclauses = 0, ccap = 0;

static void addn(int *lits, int n)
{
    if (n == 0) { fprintf(stderr, "empty clause\n"); exit(2); }
    if (nclauses == ccap) {
        ccap = ccap ? ccap * 2 : 1024;
        cbuf = realloc(cbuf, sizeof(int *) * ccap);
        clen = realloc(clen, sizeof(int) * ccap);
        if (!cbuf || !clen) { fprintf(stderr, "oom\n"); exit(2); }
    }
    cbuf[nclauses] = malloc(sizeof(int) * n);
    memcpy(cbuf[nclauses], lits, sizeof(int) * n);
    clen[nclauses] = n;
    nclauses++;
}

static void addc(int a, int b, int c, int d)
{
    int lits[4], n = 0;
    if (a) lits[n++] = a;
    if (b) lits[n++] = b;
    if (c) lits[n++] = c;
    if (d) lits[n++] = d;
    addn(lits, n);
}

/* clause with an arbitrary number of literals */
static void addbig(int *lits, int n) { addn(lits, n); }

static int newvar(void) { return ++nvars; }

/* pair index for (i,j), i<j */
static int pidx(int i, int j, int n)
{
    return i * (2 * n - i - 1) / 2 + (j - i - 1);
}

int main(int argc, char **argv)
{
    if (argc < 7) {
        fprintf(stderr, "usage: %s <n> <d> <maxC> <prefix.txt> <inputs.txt> <out.cnf>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    int d = atoi(argv[2]);
    int maxC = atoi(argv[3]);
    const char *prefixf = argv[4];
    const char *inputsf = argv[5];
    const char *outf = argv[6];

    /* ---- read the inputs ------------------------------------------------- */
    int *inp = NULL, ninp = 0, cap = 0;
    {
        FILE *f = fopen(inputsf, "r");
        if (!f) { perror(inputsf); return 1; }
        long long v;
        while (fscanf(f, "%lld", &v) == 1) {
            if (ninp == cap) { cap = cap ? cap * 2 : 1024; inp = realloc(inp, sizeof(int) * cap); }
            inp[ninp++] = (int)(v & ((1LL << n) - 1));
        }
        fclose(f);
    }

    int NP = n * (n - 1) / 2;                 /* comparator slots per layer */
    long G = (long)NP * d;                    /* total comparator variables  */

    /* variable blocks */
    long base_g = 1;                  /* SAT variables are 1-based */
    long base_u = base_g + G;                         /* u[i][k]  */
    long base_u_size = (long)n * d;
    long base_v = base_u + base_u_size;               /* v[m][i][k] */
    long base_v_size = (long)ninp * n * (d + 1);
    long base_s = base_v + base_v_size;                 /* Sinz counter */
    long base_s_size = G * (maxC + 1);

    nvars = base_s + base_s_size - 1;

    /* ---- read the prefix ------------------------------------------------- */
    int **pref = calloc(d, sizeof(int *));
    int *preflen = calloc(d, sizeof(int));
    int have_prefix = 0;
    static int have_layer[256];
    /* prefix parser: each non-# line = one layer, pairs of ints */
    {
        FILE *f = fopen(prefixf, "r");
        char line[4096];
        int layer = 0;
        while (fgets(line, sizeof(line), f) && layer < d) {
            if (line[0] == '#') { layer++; continue; }
            have_prefix = 1;
            if (layer < 256) have_layer[layer] = 1;
            int vals[512], nv = 0;
            char *p = line;
            while (*p && nv < 512) {
                while (*p == ' ' || *p == '\t' || *p == '\n' || *p == ',') p++;
                if (!*p || *p == '\n') break;
                char *end;
                long v = strtol(p, &end, 10);
                if (end == p) { p++; continue; }
                vals[nv++] = (int)v;
                p = end;
            }
            preflen[layer] = nv / 2;
            pref[layer] = malloc(sizeof(int) * (preflen[layer] * 2 + 2));
            for (int i = 0; i < preflen[layer]; i++) {
                pref[layer][2 * i] = vals[2 * i];
                pref[layer][2 * i + 1] = vals[2 * i + 1];
            }
            layer++;
        }
        fclose(f);
    }

    if (getenv("SDEBUG")) { fprintf(stderr, "have_prefix=%d lens:", have_prefix); for (int k=0;k<d;k++) fprintf(stderr, " %d", preflen[k]); fprintf(stderr, "\n"); }
    /* ---- once: no two comparators in one layer share a channel ----------- */
    for (int k = 0; k < d; k++)
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                for (int j2 = j + 1; j2 < n; j2++) {
                    int g1 = (int)(base_g + (long)k * NP + pidx(i, j, n));
                    int g2 = (int)(base_g + (long)k * NP + pidx(i, j2, n));
                    addc(-g1, -g2, 0, 0);
                }
    for (int k = 0; k < d; k++)
        for (int j = 0; j < n; j++)
            for (int i = 0; i < j; i++)
                for (int i2 = i + 1; i2 < j; i2++) {
                    int g1 = (int)(base_g + (long)k * NP + pidx(i, j, n));
                    int g2 = (int)(base_g + (long)k * NP + pidx(i2, j, n));
                    addc(-g1, -g2, 0, 0);
                }

    /* ---- u[i][k] = OR_j g[i][j][k] --------------------------------------- */
    for (int k = 0; k < d; k++)
        for (int i = 0; i < n; i++) {
            int u = (int)(base_u + (long)k * n + i);
            for (int j = 0; j < n; j++) {
                if (j == i) continue;
                int a = i < j ? i : j, b = i < j ? j : i;
                int g = (int)(base_g + (long)k * NP + pidx(a, b, n));
                addc(-g, u, 0, 0);
            }
            /* u -> (g_j1 | g_j2 | ...): channel i is used by SOME comparator.
             * (Writing "-u | g_j" for every j would demand that EVERY
             * comparator on channel i exists -- that contradicts the "once"
             * constraint and made the whole instance UNSAT.) */
            {
                int lits[64]; int nl = 0;
                lits[nl++] = -u;
                for (int j = 0; j < n; j++) {
                    if (j == i) continue;
                    int a = i < j ? i : j, b = i < j ? j : i;
                    lits[nl++] = (int)(base_g + (long)k * NP + pidx(a, b, n));
                }
                addbig(lits, nl);
            }
        }

    /* ---- update + sorts per input ---------------------------------------- */
    for (int m = 0; m < ninp; m++) {
        for (int k = 0; k <= d; k++)
            for (int i = 0; i < n; i++) {
                long v = base_v + ((long)m * n + i) * (d + 1) + k;
                if (k == 0) {
                    /* v[m][i][0] = input bit */
                    if ((inp[m] >> i) & 1) addc((int)v, 0, 0, 0);
                    else addc(-(int)v, 0, 0, 0);
                }
            }
        for (int k = 1; k <= d; k++) {
            int kk = k - 1;
            for (int i = 0; i < n; i++) {
                long vi = base_v + ((long)m * n + i) * (d + 1) + k;
                long vip = base_v + ((long)m * n + i) * (d + 1) + kk;
                int u = (int)(base_u + (long)kk * n + i);
                /* NOT u -> v[m][i][k] == v[m][i][k-1]
                 * (a channel that is NOT touched by a comparator keeps its
                 * value; a touched channel gets the min/max instead).
                 * The implication used to be reversed -- u -> unchanged --
                 * which forced every participating channel to keep its old
                 * value and made the whole instance UNSAT. */
                addc(u, -(int)vi, (int)vip, 0);
                addc(u, (int)vi, -(int)vip, 0);
                for (int j = 0; j < n; j++) {
                    if (j == i) continue;
                    int a = i < j ? i : j, b = i < j ? j : i;
                    int g = (int)(base_g + (long)kk * NP + pidx(a, b, n));
                    long vj = base_v + ((long)m * n + j) * (d + 1) + kk;
                    /* comparator (i,j) with i<j:  v_i^k = a AND b,  v_j^k = a OR b
                     * where a = v_i^{k-1}, b = v_j^{k-1}.  Each is three clauses. */
                    {
                        int VA = (int)vip, VB = (int)vj;   /* the two incoming values */
                        int VI = (int)vi,  VJ = (int)vj;   /* NOTE: vj is used for
                             * both "channel j after k" and "value of channel j
                             * before k" -- they are distinct variables, so the
                             * two are separated explicitly below. */
                        (void)VI; (void)VJ;
                    }
                    {
                        int A = (int)vip;                 /* v_i^{k-1} */
                        int B = (int)vj;                  /* v_j^{k-1} */
                        int VI = (int)vi;                 /* v_i^k      */
                        int VJ = (int)(base_v + ((long)m * n + j) * (d + 1) + k);
                        if (i < j) {
                            /* VI = A & B ; VJ = A | B */
                            addc(-g, -VI, A, 0); addc(-g, -VI, B, 0);
                            addc(-g, VI, -A, -B);
                            /* no clauses for VJ here: channel j's value is
                             * fully determined by the iteration with i == j */
                        } else {
                            /* channel i is the LARGER one: VI = A | B */
                            addc(-g, -A, VI, 0); addc(-g, -B, VI, 0);
                            addc(-g, -VI, A, B);   /* VI -> A|B */
                        }
                    }
                }
            }
        }
        /* sorts: output is non-decreasing */
        for (int i = 0; i + 1 < n; i++) {
            long v1 = base_v + ((long)m * n + i) * (d + 1) + d;
            long v2 = base_v + ((long)m * n + i + 1) * (d + 1) + d;
            addc(-(int)v1, (int)v2, 0, 0);
        }
    }

    /* ---- cardinality: at most maxC comparators (Sinz) --------------------- */
    for (long i = 0; i < G; i++) {
        int x = (int)(base_g + i);
        long cur = base_s + (long)i * (maxC + 1);          /* row of x_i      */
        if (i == 0) { addc(-x, (int)(cur + 1), 0, 0); continue; }
        long prev = base_s + (long)(i - 1) * (maxC + 1);   /* row of x_{i-1} */
        addc(-x, (int)(cur + 1), 0, 0);
        /* s[i][j] means "at least j of x_0..x_i are true" (j = 1..maxC).
         * x_i together with "at least j before" implies "at least j+1 after",
         * so the clause has to point at s[i][j+1] -- pointing at s[i][j] was
         * an off-by-one that made the counter too weak. */
        for (int j = 1; j < maxC; j++)
            addc(-x, -(int)(prev + j), (int)(cur + j + 1), 0);
        for (int j = 1; j <= maxC; j++)
            addc(-(int)(prev + j), (int)(cur + j), 0, 0);
        addc(-x, -(int)(prev + maxC), 0, 0);
    }

    /* ---- prefix fixing ---------------------------------------------------- */
    if (have_prefix) {
        for (int k = 0; k < d; k++) {
            /* Only layers that were actually given in the prefix file are
             * fixed.  Fixing the others to "empty" collapsed the whole
             * network into a single layer -- which made the prefix=layer1
             * instance UNSAT even at maxC=71 (the baseline is a witness). */
            if (!have_layer[k]) continue;
            int used[n][n];
            memset(used, 0, sizeof(used));
            for (int i = 0; i < preflen[k]; i++) {
                int a = pref[k][2 * i], b = pref[k][2 * i + 1];
                if (a > b) { int t = a; a = b; b = t; }
                if (a < 0 || b >= n) { fprintf(stderr, "bad prefix\n"); exit(2); }
                used[a][b] = 1;
            }
            for (int a = 0; a < n; a++)
                for (int b = a + 1; b < n; b++) {
                    int g = (int)(base_g + (long)k * NP + pidx(a, b, n));
                    if (used[a][b]) addc(g, 0, 0, 0);
                    else addc(-g, 0, 0, 0);
                }
        }
    }

    /* ---- write DIMACS ----------------------------------------------------- */
    out = fopen(outf, "w");
    if (!out) { perror(outf); return 1; }
    fprintf(out, "p cnf %ld %ld\n", nvars, nclauses);
    for (long i = 0; i < nclauses; i++) {
        for (int k = 0; k < clen[i]; k++) fprintf(out, "%d ", cbuf[i][k]);
        fprintf(out, "0\n");
    }
    for (long i = 0; i < nclauses; i++) free(cbuf[i]);
    fclose(out);
    fprintf(stderr, "sortsat: n=%d d=%d maxC=%d inputs=%d vars=%ld clauses=%ld\n",
            n, d, maxC, ninp, nvars, nclauses);
    return 0;
}
