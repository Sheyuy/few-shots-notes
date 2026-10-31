/* circ36.c -- exhaustive search over circulant (4,6)-Ramsey graphs on 36 vertices.
 *
 * A circulant graph on Z_36 is determined by a symmetric connection set
 * S = {-S} subset of {1..18}.  Enumerate all such sets and test each graph for
 *   (a) no 4-clique
 *   (b) no independent set of size 6  (equivalently: the complement has no K6)
 * Both tests use pruned backtracking clique search.
 *
 * usage: circ36 [maxdeg] [out.txt]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define N 36
typedef uint64_t mask;

static mask adj[N];

static int has_clique(mask cand, int need, int depth){
    if(need==0) return 1;
    if(__builtin_popcountll(cand) < need) return 0;
    while(cand){
        int v=__builtin_ctzll(cand);
        cand &= cand-1;
        if(has_clique(cand & adj[v], need-1, depth+1)) return 1;
    }
    return 0;
}

int main(int argc,char**argv){
    int maxdeg = argc>1?atoi(argv[1]):10;
    const char *out = argc>2?argv[2]:NULL;
    mask full = ((mask)1<<N)-1;
    long tested=0, k4bad=0, k6bad=0;

    /* connection sets: choose from 1..18 (i and 36-i are the same edge) */
    int pos[19];
    for(int i=0;i<=18;i++) pos[i]= i? 1:0;

    /* enumerate by a simple recursive subset generator over indices 1..18 */
    int S[19], slen=0;
    /* iterative: treat the set as a bitmask over 18 bits */
    unsigned long long limit = 1ULL<<18;
    for(unsigned long long m=1; m<limit; m++){
        int deg=0; for(int i=0;i<18;i++) if(m>>i&1) deg++;
        if(deg>maxdeg) continue;
        if(deg<2) continue;
        tested++;
        for(int i=0;i<N;i++) adj[i]=0;
        for(int i=0;i<18;i++) if(m>>i&1){
            int d=i+1;                       /* edges i+1 and 36-(i+1) */
            for(int v=0;v<N;v++){
                if(v==i+1 || v==N-(i+1)) continue;
                adj[v]|=(mask)1<<(i+1);
                adj[i+1]|=(mask)1<<v;
                adj[v]|=(mask)1<<(N-(i+1));
                adj[N-(i+1)]|=((mask)1<<v);
            }
        }
        /* (a) no K4 */
        if(has_clique(full,4,0)){ k4bad++; continue; }
        /* (b) no independent 6  <=>  complement has no K6 */
        mask comp[N];
        for(int i=0;i<N;i++) comp[i]= full & ~adj[i] & ~((mask)1<<i);
        /* clique search on the complement */
        int found6=0;
        /* simple recursive check */
        int stack[8];
        /* iterative deepening via recursion helper */
        int ok=1;
        /* use has_clique-like routine on complement */
        {
            mask cands[8]; int nd=0;
            /* do a manual recursion */
            mask c=full; int level=0; int found=0;
            /* simple recursive lambda replacement */
            struct { int dummy; } unused;
            (void)unused;
            /* do it iteratively with explicit recursion */
            int sel[8];
            int rec(mask cand, int need){
                if(need==0) return 1;
                while(cand){
                    int v=__builtin_ctzll(cand); cand&=cand-1;
                    sel[need<8?0:0]=v;
                    if(rec(cand & comp[v], need-1)) return 1;
                }
                return 0;
            }
            found = rec(full,6);
            ok = !found;
        }
        if(!ok){ k6bad++; continue; }
        printf("FOUND circulant on 36 vertices, connection set:");
        for(int i=0;i<18;i++) if(m>>i&1) printf(" %d",i+1);
        printf("\n");
        if(out){
            FILE *f=fopen(out,"w");
            fprintf(f,"{\"adjacency\":[");
            for(int i=0;i<N;i++){ if(i)fprintf(f,","); fprintf(f,"\"");
                for(int j=0;j<N;j++) fputc((adj[i]>>j&1ULL)?'1':'0', f);
                fprintf(f,"\""); }
            fprintf(f,"]}\n");
            fclose(f);
        }
        fflush(stdout);
    }
    printf("tested=%ld  rejected K4=%ld  rejected K6=%ld\n",tested,k4bad,k6bad);
    return 0;
}
