#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M7 2187
#define N0 512
#define MAXR 300
#define MAXT 512
#define NW 8              /* 8*64 = 512 triple bits */

static short third7[M7][M7];
static int pow3[10];

static int digit7(int code,int c){ return (code/pow3[c])%3; }

/* exact min hitting set over triples using iterative deepening */
static int ntrip, nw;
static int tripA[MAXT], tripB[MAXT], tripC[MAXT];
static unsigned long long vmask[M7][NW];
static unsigned long long fullmask[NW];

static int dfs(unsigned long long cov[NW], int budget, int *chosen, int depth, int limit) {
    int all = 1;
    for (int w = 0; w < nw; w++) if ((cov[w] & fullmask[w]) != fullmask[w]) { all = 0; break; }
    if (all) return depth;               /* success, depth = #chosen */
    if (budget == 0) return -1;
    int t = -1;
    for (int w = 0; w < nw; w++) {
        unsigned long long u = (~cov[w]) & fullmask[w];
        if (u) { t = w * 64 + __builtin_ctzll(u); break; }
    }
    if (t < 0 || t >= ntrip) return -1;
    int vs[3] = { tripA[t], tripB[t], tripC[t] };
    for (int q = 0; q < 3; q++) {
        int v = vs[q];
        unsigned long long ncov[NW];
        for (int w = 0; w < nw; w++) ncov[w] = cov[w] | vmask[v][w];
        chosen[depth] = v;
        int r = dfs(ncov, budget - 1, chosen, depth + 1, limit);
        if (r >= 0) return r;
    }
    return -1;
}

int main(int argc, char **argv) {
    int maxk = (argc > 1) ? atoi(argv[1]) : 12;
    pow3[0] = 1; for (int k = 1; k < 10; k++) pow3[k] = pow3[k-1] * 3;
    for (int a = 0; a < M7; a++) for (int b = 0; b < M7; b++) {
        int x = a, y = b, z = 0, pw = 1;
        for (int k = 0; k < 7; k++) { int s = ((x%3)+(y%3))%3; int c = (3-s)%3; z += c*pw; pw *= 3; x /= 3; y /= 3; }
        third7[a][b] = (short)z;
    }

    FILE *f = fopen("baseline_pts.txt", "r");
    if (!f) { fprintf(stderr, "no baseline\n"); return 1; }
    int B[MAXR], Bn = 0; static char seen[M7];
    for (int t = 0; t < N0; t++) {
        int v[8]; for (int k = 0; k < 8; k++) if (fscanf(f, "%d", &v[k]) != 1) { fprintf(stderr, "parse\n"); return 1; }
        if (v[0] != 0) { int c = 0; for (int k = 1; k < 8; k++) c += v[k]*pow3[k-1]; if (!seen[c]) { seen[c]=1; B[Bn++]=c; } }
    }
    fclose(f);
    printf("|B|=%d\n", Bn);

    int bestScore = 0, bestBi = -1, bestMaxcap = 0, bestRn = 0, bestK = 0;
    int bestC[MAXR], bestB[MAXR];

    for (int bi = 0; bi < Bn; bi++) {
        int Bp[MAXR], Bpn = 0;
        for (int i = 0; i < Bn; i++) if (i != bi) Bp[Bpn++] = B[i];
        /* build R */
        static char forb[M7]; memset(forb, 0, sizeof(forb));
        for (int i = 0; i < Bpn; i++) forb[Bp[i]] = 1;
        for (int i = 0; i < Bpn; i++) for (int j = i+1; j < Bpn; j++) forb[third7[Bp[i]][Bp[j]]] = 1;
        int R[MAXR], Rn = 0; static int pos[M7];
        for (int x = 0; x < M7; x++) pos[x] = -1;
        for (int x = 0; x < M7; x++) if (!forb[x]) { pos[x] = Rn; R[Rn++] = x; }
        /* enumerate triples */
        ntrip = 0;
        for (int i = 0; i < Rn; i++) for (int j = i+1; j < Rn; j++) {
            int r = third7[R[i]][R[j]];
            int k = pos[r];
            if (k > j) { if (ntrip < MAXT) { tripA[ntrip]=i; tripB[ntrip]=j; tripC[ntrip]=k; ntrip++; } }
        }
        nw = (ntrip + 63) / 64; if (nw == 0) nw = 1; if (nw > NW) { printf("too many triples %d\n", ntrip); return 1; }
        for (int w = 0; w < nw; w++) fullmask[w] = ~0ULL;
        if (ntrip % 64) fullmask[nw-1] = (1ULL << (ntrip % 64)) - 1;
        memset(vmask, 0, sizeof(vmask));
        for (int t = 0; t < ntrip; t++) {
            int vs[3] = {tripA[t], tripB[t], tripC[t]};
            for (int q = 0; q < 3; q++) vmask[vs[q]][t/64] |= (1ULL << (t%64));
        }
        /* IDDFS */
        int minhs = -1, chosen[MAXR];
        for (int k = 0; k <= maxk; k++) {
            unsigned long long cov[NW]; memset(cov, 0, sizeof(cov));
            int r = dfs(cov, k, chosen, 0, k);
            if (r >= 0) { minhs = r; break; }
        }
        int maxcap = (minhs < 0) ? -1 : (Rn - minhs);
        int score = 2 * Bpn + maxcap;
        if (score > bestScore) {
            bestScore = score; bestBi = bi; bestMaxcap = maxcap; bestRn = Rn; bestK = minhs;
            memcpy(bestB, Bp, Bpn*sizeof(int));
            /* rebuild best C = R minus chosen */
            static char inch[M7]; memset(inch, 0, sizeof(inch));
            for (int q = 0; q < minhs; q++) inch[chosen[q]] = 1;
            int m = 0; for (int z = 0; z < Rn; z++) if (!inch[z]) bestC[m++] = R[z];
            printf("  best score=%d (remove b idx %d, |B'|=%d |R'|=%d triples=%d minHS=%d maxcap=%d)\n",
                   score, bi, Bpn, Rn, ntrip, minhs, maxcap); fflush(stdout);
        }
    }
    printf("BEST: score=%d |R'|=%d minHS=%d maxcap=%d\n", bestScore, bestRn, bestK, bestMaxcap);
    if (bestScore >= 513) {
        printf("*** SUCCESS %d ***\n", bestScore);
        FILE *o = fopen("candidate_pts.txt", "w");
        int Bpn = 0; for (int i = 0; i < Bn; i++) if (i != bestBi) bestB[Bpn++] = B[i];
        for (int i = 0; i < Bpn; i++) {
            int v[7]; int x = bestB[i]; for (int k = 0; k < 7; k++){ v[k] = x%3; x/=3; }
            fprintf(o, "1 %d %d %d %d %d %d %d\n", v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
            fprintf(o, "2 %d %d %d %d %d %d %d\n", v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
        }
        int m = 0;
        /* bestC was filled with maxcap points */
        for (int i = 0; i < bestMaxcap; i++) {
            int v[7]; int x = bestC[i]; for (int k = 0; k < 7; k++){ v[k] = x%3; x/=3; }
            fprintf(o, "0 %d %d %d %d %d %d %d\n", v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
            m++;
        }
        fclose(o);
        printf("wrote candidate_pts.txt with %d points (%d B + %d C)\n", 2*Bpn + m, 2*Bpn, m);
    }
    return 0;
}
