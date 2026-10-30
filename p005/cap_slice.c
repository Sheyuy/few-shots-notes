#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define D7 7
#define M7 2187
#define NFULL 512
#define MAXR 900

static short third7[M7][M7];

static int enc7(const int *v) { int x = 0; for (int k = D7 - 1; k >= 0; k--) x = x * 3 + v[k]; return x; }
static void dec7(int x, int *v) { for (int k = 0; k < D7; k++) { v[k] = x % 3; x /= 3; } }

static unsigned long rng_state;
static unsigned int xrand(void) {
    rng_state ^= rng_state << 13; rng_state ^= rng_state >> 7; rng_state ^= rng_state << 17;
    return (unsigned int)(rng_state >> 11);
}
static void shuffle(int *a, int n) {
    for (int i = n - 1; i > 0; i--) { int j = xrand() % (i + 1); int t = a[i]; a[i] = a[j]; a[j] = t; }
}

/* randomized greedy maximal cap within candidate set R (size Rn) */
static int maxcap(const int *R, int Rn, int restarts, int *bestC, int *ord, char *mem, int *cur) {
    int bestn = 0;
    for (int r = 0; r < restarts; r++) {
        memcpy(ord, R, Rn * sizeof(int));
        shuffle(ord, Rn);
        memset(mem, 0, M7);
        int ncur = 0;
        for (int i = 0; i < Rn; i++) {
            int p = ord[i], ok = 1;
            for (int j = 0; j < ncur; j++) {
                if (mem[third7[p][cur[j]]]) { ok = 0; break; }
            }
            if (ok) { mem[p] = 1; cur[ncur++] = p; }
        }
        if (ncur > bestn) { bestn = ncur; memcpy(bestC, cur, ncur * sizeof(int)); }
    }
    return bestn;
}

/* build R = complement of (B union closure(B)) */
static int buildR(const int *B, int Bn, int *R) {
    static char forb[M7];
    memset(forb, 0, sizeof(forb));
    for (int i = 0; i < Bn; i++) forb[B[i]] = 1;
    for (int i = 0; i < Bn; i++)
        for (int j = i + 1; j < Bn; j++) forb[third7[B[i]][B[j]]] = 1;
    int n = 0;
    for (int x = 0; x < M7; x++) if (!forb[x]) R[n++] = x;
    return n;
}

int main(int argc, char **argv) {
    int restarts = (argc > 1) ? atoi(argv[1]) : 40;
    rng_state = (argc > 2) ? (unsigned long)atoll(argv[2]) : 88172645463325252UL;
    if (!rng_state) rng_state = 1;

    /* build third7 table */
    for (int a = 0; a < M7; a++) {
        for (int b = 0; b < M7; b++) {
            int x = a, y = b, z = 0, pw = 1;
            for (int k = 0; k < D7; k++) {
                int s = ((x % 3) + (y % 3)) % 3;
                int c = (3 - s) % 3;
                z += c * pw; pw *= 3; x /= 3; y /= 3;
            }
            third7[a][b] = (short)z;
        }
    }

    /* read baseline */
    FILE *f = fopen("baseline_pts.txt", "r");
    if (!f) { fprintf(stderr, "no baseline_pts.txt\n"); return 1; }
    int Bv[NFULL][D7], nB = 0;
    int Cv[NFULL][D7], nC = 0;
    for (int t = 0; t < NFULL; t++) {
        int v[8];
        for (int k = 0; k < 8; k++) if (fscanf(f, "%d", &v[k]) != 1) { fprintf(stderr, "parse fail\n"); return 1; }
        if (v[0] == 0) { for (int k = 0; k < D7; k++) Cv[nC][k] = v[k + 1]; nC++; }
        else { for (int k = 0; k < D7; k++) Bv[nB][k] = v[k + 1]; nB++; }
    }
    fclose(f);
    /* B: dedupe (x0=1 and x0=2 give same projection) */
    int B[MAXR], Bn = 0;
    {
        static char seen[M7]; memset(seen, 0, sizeof(seen));
        for (int i = 0; i < nB; i++) { int c = enc7(Bv[i]); if (!seen[c]) { seen[c] = 1; B[Bn++] = c; } }
    }
    printf("|B|=%d  (raw B points %d)  |C|=%d\n", Bn, nB, nC);

    int *R = malloc(M7 * sizeof(int));
    int *bestC = malloc(M7 * sizeof(int));
    int *ord = malloc(M7 * sizeof(int));
    int *cur = malloc(M7 * sizeof(int));
    char *mem = malloc(M7);

    int Rn = buildR(B, Bn, R);
    int mc = maxcap(R, Rn, restarts, bestC, ord, mem, cur);
    printf("baseline: |B|=%d |R|=%d maxcap(R)=%d  score=2|B|+maxcap=%d\n", Bn, Rn, mc, 2 * Bn + mc);

    /* single removal scan */
    int best_score = 0, best_b = -1, best_mc = 0, best_Rn = 0;
    int bestB[MAXR], bestB_mc[MAXR], bestBn = 0;
    (void)bestB_mc;
    for (int bi = 0; bi < Bn; bi++) {
        int Bp[MAXR], Bpn = 0;
        for (int i = 0; i < Bn; i++) if (i != bi) Bp[Bpn++] = B[i];
        int Rpn = buildR(Bp, Bpn, R);
        int m = maxcap(R, Rpn, restarts, bestC, ord, mem, cur);
        int score = 2 * Bpn + m;
        if (score > best_score) {
            best_score = score; best_b = bi; best_mc = m; best_Rn = Rpn;
            memcpy(bestB, Bp, Bpn * sizeof(int)); bestBn = Bpn;
            printf("  new best score=%d  (removed b=%d, |B'|=%d |R'|=%d maxcap=%d)\n",
                   score, B[bi], Bpn, Rpn, m);
            fflush(stdout);
        }
    }
    printf("BEST single-removal: score=%d  |B'|=%d  maxcap=%d  |R'|=%d\n", best_score, bestBn, best_mc, best_Rn);
    if (best_score >= 513) {
        /* output reconstructed certificate: B' x {1,2} plus bestC x {0} */
        printf("*** FOUND score %d (need >=513) ***\n", best_score);
        /* rebuild R for bestB and re-run maxcap with many restarts to get bestC */
        int Rpn = buildR(bestB, bestBn, R);
        int m2 = maxcap(R, Rpn, 400, bestC, ord, mem, cur);
        printf("re-optimized maxcap=%d -> total=%d\n", m2, 2 * bestBn + m2);
        FILE *o = fopen("candidate_pts.txt", "w");
        for (int i = 0; i < bestBn; i++) {
            int v[D7]; dec7(bestB[i], v);
            fprintf(o, "1 %d %d %d %d %d %d %d\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
            fprintf(o, "2 %d %d %d %d %d %d %d\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
        }
        for (int i = 0; i < m2; i++) {
            int v[D7]; dec7(bestC[i], v);
            fprintf(o, "0 %d %d %d %d %d %d %d\n", v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
        }
        fclose(o);
        printf("wrote candidate_pts.txt with %d points\n", 2 * bestBn + m2);
    }
    return 0;
}
