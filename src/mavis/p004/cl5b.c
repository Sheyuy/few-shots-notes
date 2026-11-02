/* cl5b.c -- "remove k, add k+1" on the 367-point Polak-Schrijver set R in C7^5.
 *
 * Goal: find a set A of points outside R such that
 *      (a) the points of A are pairwise non-adjacent (so A is independent), and
 *      (b) |U| = |union of N_R(a) over a in A|  <  |A|,
 * then R \ U  +  A is a strictly larger independent set.
 *
 * The previous brute force (cl5.c) enumerated, for every compatible pair
 * (i,j), ALL (K-2)-subsets of their common compatible neighbourhood.  That is
 * C(|C|,K-2) per pair and blew up immediately for K >= 5 (468946 pairs for
 * K=5, 2.4M for K=6, ... and it still did not finish 20 minutes).
 *
 * Here we do it properly:
 *   for every compatible pair (i,j) with |U| = |N_R(i) | N_R(j)| <= K-1:
 *       C' = { c : c compatible with i and j, and N_R(c) subset of U }
 *       compute a maximum independent set of the compatibility graph on C'
 *       (branch and bound over bitsets)
 *       the whole candidate set is then {i,j} | MIS, whose neighbourhood union
 *       is still inside U, so the improvement test is |MIS|+2 > |U|.
 *
 * Any independent set inside C' works, so the branch and bound may stop as
 * soon as it has found one of size >= K-2 (which already certifies the
 * improvement); the full maximum is computed only when that fails, to report
 * the best achievable gain.
 *
 * Build: gcc -O2 -o cl5b cl5b.c
 * Usage: ./cl5b <K> <R5.txt> [first_pair] [last_pair]
 *        K = how many points we want to add (>=2); the search then looks for
 *        |A| = K with |U| <= K-1.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define DIM 5
#define CYC 7
#define NV 16807
#define NBR 242
#define MAXC 20000
#define WMAX ((MAXC + 63) / 64)

static uint8_t dig[NV][DIM];
static uint16_t nbr[NV][NBR + 1];
static uint8_t inR[NV];
static int16_t nbR[NV][12];
static uint8_t cnR[NV];

static void gen(void)
{
    for (int v = 0; v < NV; v++) {
        int t = v;
        for (int k = 4; k >= 0; k--) { dig[v][k] = (uint8_t)(t % 7); t /= 7; }
    }
    for (int v = 0; v < NV; v++) {
        int m = 0;
        for (int e = 0; e <= NBR; e++) {
            int x = e, idx = 0, val[5];
            for (int k = 0; k < 5; k++) { int f = x % 3; x /= 3; val[k] = (dig[v][k] - 1 + f + 7) % 7; }
            for (int k = 0; k < 5; k++) idx = idx * 7 + val[k];
            nbr[v][m++] = (uint16_t)idx;
        }
    }
}

static inline int adj(int a, int b)
{
    for (int k = 0; k < 5; k++) {
        int d = dig[a][k] - dig[b][k];
        if (d == 0 || d == 1 || d == -1 || d == 6 || d == -6) continue;
        return 0;
    }
    return 1;
}

/* ---- bitset helpers over candidate indices -------------------------- */
static uint64_t comp[MAXC][WMAX];      /* compatibility: comp[i][j] = 1 iff i,j may coexist */
static uint64_t setA[WMAX], setB[WMAX];
static int ncand;

static inline void bs_set(uint64_t *b, int i) { b[i >> 6] |= 1ULL << (i & 63); }
static inline int bs_get(const uint64_t *b, int i) { return (b[i >> 6] >> (i & 63)) & 1ULL; }
static inline int bs_pop(const uint64_t *b, int nw)
{
    int c = 0;
    for (int w = 0; w < nw; w++) c += __builtin_popcountll(b[w]);
    return c;
}

/* Maximum independent set of the compatibility graph restricted to `S`
 * (a bitset over candidate indices).  Returns the size and writes the chosen
 * indices into `out`.  `stopAt` >= 0 lets us stop as soon as a set of that
 * size is found. */
static int bestSize;
static int bestSet[MAXC];
static int curSet[MAXC];
static int curSize;

static void mis_rec(uint64_t *S, int nw, int stopAt)
{
    int cnt = bs_pop(S, nw);
    if (cnt + curSize <= bestSize) return;          /* cannot beat the incumbent */
    if (stopAt >= 0 && cnt + curSize < stopAt) return;   /* cannot even reach the target */
    /* pick the first vertex of S */
    int pick = -1;
    for (int w = 0; w < nw && pick < 0; w++) {
        if (S[w]) pick = (w << 6) + __builtin_ctzll(S[w]);
    }
    if (pick < 0) {
        /* S empty -> S itself is independent */
        if (curSize > bestSize) {
            bestSize = curSize;
            memcpy(bestSet, curSet, sizeof(int) * (size_t)curSize);
        }
        return;
    }
    /* branch 1: include pick (and drop its neighbours) */
    {
        uint64_t T[WMAX];
        memcpy(T, S, sizeof(uint64_t) * (size_t)nw);
        for (int w = 0; w < nw; w++) T[w] &= ~comp[pick][w];
        T[pick >> 6] &= ~(1ULL << (pick & 63));
        curSet[curSize++] = pick;
        mis_rec(T, nw, stopAt);
        curSize--;
        if (stopAt >= 0 && bestSize >= stopAt) return;
    }
    /* branch 2: exclude pick */
    {
        uint64_t T[WMAX];
        memcpy(T, S, sizeof(uint64_t) * (size_t)nw);
        T[pick >> 6] &= ~(1ULL << (pick & 63));
        mis_rec(T, nw, stopAt);
    }
}

int main(int argc, char **argv)
{
    int K = (argc > 1) ? atoi(argv[1]) : 4;
    const char *rf = (argc > 2) ? argv[2] : "R5.txt";
    long long firstPair = (argc > 3) ? atoll(argv[3]) : 0;
    long long lastPair = (argc > 4) ? atoll(argv[4]) : -1;
    int countOnly = (argc > 5 && !strcmp(argv[5], "count"));

    gen();
    FILE *fp = fopen(rf, "r");
    if (!fp) { perror(rf); return 1; }
    char l[64];
    while (fgets(l, sizeof l, fp)) {
        int i = 0, nd = 0;
        for (const char *q = l; *q; q++) {
            if (*q < '0' || *q > '6') break;
            i = i * 7 + (*q - '0'); nd++;
        }
        if (nd == 5) inR[i] = 1;
    }
    fclose(fp);

    for (int x = 0; x < NV; x++) {
        if (inR[x]) { cnR[x] = 0; continue; }
        int c = 0;
        for (int j = 0; j <= NBR; j++) { int y = nbr[x][j]; if (inR[y] && c < 12) nbR[x][c++] = (int16_t)y; }
        cnR[x] = (uint8_t)c;
    }
    static int cand[MAXC];
    ncand = 0;
    for (int x = 0; x < NV; x++) if (!inR[x] && cnR[x] <= K) cand[ncand++] = x;
    fprintf(stderr, "candidates(|N_R|<=%d): %d\n", K, ncand);
    int W = (ncand + 63) / 64;
    memset(comp, 0, sizeof(uint64_t) * (size_t)MAXC * (size_t)W);

    /* ---- pass 1: build the compatibility matrix over all compatible pairs ---- */
    static int *pairI, *pairJ;
    long long ncap = 0;
    for (int i = 0; i < ncand; i++)
        for (int j = i + 1; j < ncand; j++) {
            if (adj(cand[i], cand[j])) continue;
            int u[12], nu = 0;
            for (int a = 0; a < cnR[cand[i]]; a++) u[nu++] = nbR[cand[i]][a];
            for (int b = 0; b < cnR[cand[j]]; b++) {
                int f = 0;
                for (int a = 0; a < nu; a++) if (u[a] == nbR[cand[j]][b]) { f = 1; break; }
                if (!f) u[nu++] = nbR[cand[j]][b];
            }
            if (nu > K - 1) continue;               /* this pair alone already costs too much */
            comp[i][j >> 6] |= 1ULL << (j & 63);
            comp[j][i >> 6] |= 1ULL << (i & 63);
            ncap++;
        }
    fprintf(stderr, "compatible pairs with |union| <= %d: %lld\n", K - 1, ncap);
    pairI = (int *)malloc(sizeof(int) * (size_t)(ncap > 0 ? ncap : 1));
    pairJ = (int *)malloc(sizeof(int) * (size_t)(ncap > 0 ? ncap : 1));
    long long npair = 0;
    for (int i = 0; i < ncand; i++)
        for (int j = i + 1; j < ncand; j++) {
            if (adj(cand[i], cand[j])) continue;
            int u[12], nu = 0;
            for (int a = 0; a < cnR[cand[i]]; a++) u[nu++] = nbR[cand[i]][a];
            for (int b = 0; b < cnR[cand[j]]; b++) {
                int f = 0;
                for (int a = 0; a < nu; a++) if (u[a] == nbR[cand[j]][b]) { f = 1; break; }
                if (!f) u[nu++] = nbR[cand[j]][b];
            }
            if (nu > K - 1) continue;
            pairI[npair] = i; pairJ[npair] = j; npair++;
        }

    if (countOnly) {
        fprintf(stdout, "candidates %d\npairs %lld\n", ncand, npair);
        return 0;
    }
    /* ---- pass 2: search the pairs in the requested range ---- */
    long long done = 0, improved = 0;
    int bestGain = 0;
    for (long long pid = firstPair; pid < npair; pid++) {
        if (lastPair >= 0 && pid > lastPair) break;
        int i = pairI[pid], j = pairJ[pid];
        int u[12], nu = 0;
        for (int a = 0; a < cnR[cand[i]]; a++) u[nu++] = nbR[cand[i]][a];
        for (int b = 0; b < cnR[cand[j]]; b++) {
            int f = 0;
            for (int a = 0; a < nu; a++) if (u[a] == nbR[cand[j]][b]) { f = 1; break; }
            if (!f) u[nu++] = nbR[cand[j]][b];
        }
        done++;
        /* C' = candidates compatible with i and j whose N_R is inside U */
        memset(setB, 0, sizeof(uint64_t) * (size_t)W);
        for (int c = 0; c < ncand; c++) {
            if (c == i || c == j) continue;
            if (!bs_get(comp[i], c) || !bs_get(comp[j], c)) continue;
            int ok = 1;
            for (int b = 0; b < cnR[cand[c]]; b++) {
                int f = 0;
                for (int a = 0; a < nu; a++) if (u[a] == nbR[cand[c]][b]) { f = 1; break; }
                if (!f) { ok = 0; break; }
            }
            if (ok) bs_set(setB, c);
        }
        int cnt = bs_pop(setB, W);
        if (cnt < K - 2) continue;                     /* not enough room */
        bestSize = 0; curSize = 0;
        memcpy(setA, setB, sizeof(uint64_t) * (size_t)W);
        mis_rec(setA, W, K - 2);
        int tot = bestSize + 2;                        /* |A| */
        int unionSize = nu;
        if (tot > unionSize) {
            improved++;
            int gain = tot - unionSize;
            if (gain > bestGain) bestGain = gain;
            printf("IMPROVEMENT: remove %d, add %d -> size %d (gain %d)\n",
                   unionSize, tot, 367 - unionSize + tot, gain);
            printf("  remove:");
            for (int a = 0; a < nu; a++) printf(" %d%d%d%d%d", dig[u[a]][0], dig[u[a]][1], dig[u[a]][2], dig[u[a]][3], dig[u[a]][4]);
            printf("\n  add:");
            for (int a = 0; a < bestSize; a++) {
                int c = bestSet[a];
                printf(" %d%d%d%d%d", dig[cand[c]][0], dig[cand[c]][1], dig[cand[c]][2], dig[cand[c]][3], dig[cand[c]][4]);
            }
            printf(" %d%d%d%d%d\n", dig[cand[i]][0], dig[cand[i]][1], dig[cand[i]][2], dig[cand[i]][3], dig[cand[i]][4]);
            printf(" %d%d%d%d%d\n", dig[cand[j]][0], dig[cand[j]][1], dig[cand[j]][2], dig[cand[j]][3], dig[cand[j]][4]);
            fflush(stdout);
            if (improved >= 20) { printf("stopping after 20 improvements\n"); return 0; }
        }
    }
    fprintf(stderr, "pairs=%lld searched=%lld improvements=%lld bestGain=%d\n", npair, done, improved, bestGain);
    return improved ? 0 : 1;
}
