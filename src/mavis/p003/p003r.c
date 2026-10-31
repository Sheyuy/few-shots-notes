/* p003r.c -- repair / construct a (4,6)-Ramsey graph on n vertices.
 *
 * The certificate IS the adjacency matrix, so edges may be flipped freely.
 * Constraint: no 4-clique and no independent set of size 6.
 * Objective: minimise (#K4 + #independent-6); 0 means a valid certificate.
 *
 * Incremental bookkeeping: flipping the edge (a,b)
 *   - every K4 containing a and b disappears        (delta -= cnt4(a,b))
 *   - every independent 6-set containing a and b appears (delta += cnt6(a,b))
 * and nothing else changes, because a vertex's adjacency to the other
 * members of a violation is untouched by that flip.
 *
 * usage: p003r <seed.json> <secs> <seed> <out.json>
 *        p003r random <n> <secs> <seed> <out.json>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

#define MAXN 40
typedef uint64_t mask;

static int n;
static mask adj[MAXN];
static int tabu_run(long n, const long *seedv, double budget, uint64_t seed, const char *out);
static int kill_move(long n);
static int find_k4(int out4[4]);
static int enum_ind4(int a,int b,int out[][4]);
static void write_out(const char *path);
static int tabu_refine(long n, long steps);
static int mem_run(long n, const long *seedv, double budget, uint64_t seed, const char *out);
static mask FULL;

static double now_s(void){struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);return ts.tv_sec+1e-9*ts.tv_nsec;}
static uint64_t rs = 88172645463325252ULL;
static uint64_t rnd(void){rs^=rs<<13;rs^=rs>>7;rs^=rs<<17;return rs;}
static int rndint(int k){return (int)(rnd()%(uint64_t)k);}

/* number of adjacent pairs among the common neighbours of a and b  = #K4 through (a,b) */
static int cnt4(int a,int b){
    mask C = adj[a] & adj[b];
    int e=0;
    mask t=C;
    while(t){ int c=__builtin_ctzll(t); t&=t-1; e+=__builtin_popcountll(adj[c]&C); }
    return e/2;
}

/* number of independent 4-subsets of the vertices non-adjacent to both a and b
 * = #independent-6 sets that would appear through (a,b) */
static int cnt6(int a,int b){
    mask S = FULL & ~(adj[a]|adj[b]|((mask)1<<a)|((mask)1<<b));
    int idx[40], m=0; mask t=S;
    while(t){ idx[m++]=__builtin_ctzll(t); t&=t-1; }
    int c=0;
    for(int i=0;i<m;i++) for(int j=i+1;j<m;j++) if(adj[idx[i]]>>idx[j]&1ULL) continue;
        else for(int k=j+1;k<m;k++) if(adj[idx[i]]>>idx[k]&1ULL) continue;
        else for(int l=k+1;l<m;l++){
            if(adj[idx[i]]>>idx[l]&1ULL) continue;
            if(adj[idx[j]]>>idx[k]&1ULL) continue;
            if(adj[idx[j]]>>idx[l]&1ULL) continue;
            if(adj[idx[k]]>>idx[l]&1ULL) continue;
            c++;
        }
    return c;
}

static long total_viol;          /* #K4 + #independent-6 */
static long total_k4, total_i6;

static void recount(void){
    /* K4's are counted through their 6 edges, independent 6-sets through
     * their 15 non-edges: cnt4 only makes sense for a present edge and
     * cnt6 only for an absent one. */
    long s4=0,s6=0;
    for(int a=0;a<n;a++) for(int b=a+1;b<n;b++){
        if(adj[a]>>b&1ULL) s4+=cnt4(a,b);
        else              s6+=cnt6(a,b);
    }
    total_k4 = s4/6; total_i6 = s6/15; total_viol = total_k4 + total_i6;
}

static void flip(int a,int b,int delta_mode){
    /* delta_mode: 1 = edge exists (removing it), 0 = edge absent (adding it) */
    int d;
    if(delta_mode) d = -cnt4(a,b) + cnt6(a,b);
    else          d = +cnt4(a,b) - cnt6(a,b);
    adj[a]^=(mask)1<<b; adj[b]^=(mask)1<<a;
    total_viol += d;
}


/* Enumerate the independent 4-subsets of the vertices non-adjacent to both a and b.
 * Each such subset S together with {a,b} is an independent 6-set that exists
 * as soon as the edge (a,b) is removed. */
static int enum_ind4(int a,int b,int out[][4]){
    mask S = FULL & ~(adj[a]|adj[b]|((mask)1<<a)|((mask)1<<b));
    int sb[40], sm=0; mask t=S;
    while(t){ sb[sm++]=__builtin_ctzll(t); t&=t-1; }
    int n=0;
    for(int i=0;i<sm;i++) for(int j=i+1;j<sm;j++){
        if(adj[sb[i]]>>sb[j]&1ULL) continue;
        for(int k=j+1;k<sm;k++){
            if(adj[sb[i]]>>sb[k]&1ULL || adj[sb[j]]>>sb[k]&1ULL) continue;
            for(int l=k+1;l<sm;l++){
                if(adj[sb[i]]>>sb[l]&1ULL || adj[sb[j]]>>sb[l]&1ULL || adj[sb[k]]>>sb[l]&1ULL) continue;
                out[n][0]=sb[i]; out[n][1]=sb[j]; out[n][2]=sb[k]; out[n][3]=sb[l]; n++;
                if(n>=64) return n;
            }
        }
    }
    return n;
}

/* Find a K4 and return its four vertices, or 0 if none is found. */
static int find_k4(int out4[4]){
    for(int tries=0; tries<24; tries++){
        int u=rndint(n);
        mask N=adj[u]; int nb[40], m=0; mask t=N;
        while(t){ nb[m++]=__builtin_ctzll(t); t&=t-1; }
        for(int i=0;i<m;i++) for(int j=i+1;j<m;j++){
            if(!(adj[nb[i]]>>nb[j]&1ULL)) continue;
            for(int k=j+1;k<m;k++){
                if(!(adj[nb[i]]>>nb[k]&1ULL) || !(adj[nb[j]]>>nb[k]&1ULL)) continue;
                out4[0]=u; out4[1]=nb[i]; out4[2]=nb[j]; out4[3]=nb[k];
                return 1;
            }
        }
    }
    return 0;
}


/* Capped count of the independent 4-subsets of the common non-neighbours of
 * a and b.  Used ONLY for comparing candidate moves (the relative order is
 * what matters); the exact cnt6() is used for the incremental bookkeeping so
 * the violation counters stay exact. */
static int cnt6_cap(int a,int b,int cap){
    mask S = FULL & ~(adj[a]|adj[b]|((mask)1<<a)|((mask)1<<b));
    int idx[40], m=0; mask t=S;
    while(t){ idx[m++]=__builtin_ctzll(t); t&=t-1; }
    int c=0;
    for(int i=0;i<m;i++) for(int j=i+1;j<m;j++) if(!(adj[idx[i]]>>idx[j]&1ULL))
        for(int k=j+1;k<m;k++) if(!(adj[idx[i]]>>idx[k]&1ULL) && !(adj[idx[j]]>>idx[k]&1ULL))
            for(int l=k+1;l<m;l++){
                if(adj[idx[i]]>>idx[l]&1ULL) continue;
                if(adj[idx[j]]>>idx[l]&1ULL) continue;
                if(adj[idx[k]]>>idx[l]&1ULL) continue;
                if(++c>=cap) return c;
            }
    return c;
}

static long g_tenure_base = 10;
static long g_shake = 0;   /* 0 = off; else the shake period in iterations */
static int  g_kill = 0;     /* 1 = enable the combined 4-clique kill move */
static int tabu_run(long n, const long *seedv, double budget, uint64_t seed, const char *out){
    rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    if(!rs) rs=88172645463325252ULL;
    memcpy(adj, seedv, sizeof(mask)*n);
    FULL = ((mask)1<<n)-1;
    recount();
    long best=total_viol;
    mask bestadj[MAXN]; memcpy(bestadj,adj,sizeof(mask)*n);
    static long tabuT[MAXN*MAXN];
    for(int i=0;i<MAXN*MAXN;i++) tabuT[i]=-1;
    double t0=now_s(); long it=0;
    fprintf(stderr,"tabu start viol=%ld (K4=%ld I6=%ld)\n",total_viol,total_k4,total_i6);
    while(now_s()-t0<budget){
        it++;
        long bi=-1,bj=-1; int bd=1<<30;
        for(int a=0;a<n;a++) for(int b=a+1;b<n;b++){
            int ex = (adj[a]>>b&1ULL)?1:0;
            int c6 = cnt6_cap(a,b,64);
            int d = ex ? (-cnt4(a,b)+c6) : (+cnt4(a,b)-c6);
            long tt = tabuT[a*MAXN+b];
            if(tt>it && !(total_viol+d<best)) continue;
            if(d<bd){ bd=d; bi=a; bj=b; }
        }
        if(bi<0){
            for(int a=0;a<n;a++) for(int b=a+1;b<n;b++){
                int ex = (adj[a]>>b&1ULL)?1:0;
                int c6 = cnt6_cap(a,b,64);
                int d = ex ? (-cnt4(a,b)+c6) : (+cnt4(a,b)-c6);
                if(d<bd){ bd=d; bi=a; bj=b; }
            }
        }
        if(bi<0) break;
        int ex = (adj[bi]>>bj&1ULL)?1:0;
        flip(bi,bj,ex);
        tabuT[bi*MAXN+bj] = it + g_tenure_base + (long)(rnd()%(g_tenure_base>4?g_tenure_base:12));
        tabuT[bj*MAXN+bi] = tabuT[bi*MAXN+bj];
        /* with probability 1/3 also apply a kill move: removes a 4-clique
         * edge and repairs the independent 6-sets that removal creates */
        if(g_kill && (rnd()%3)==0) kill_move(n);
        if(total_viol<best){
            best=total_viol; memcpy(bestadj,adj,sizeof(mask)*n);
            fprintf(stderr,"tabu viol=%ld (K4=%ld I6=%ld) it=%ld (%.0fs)\n",best,total_k4,total_i6,it,now_s()-t0);
            fflush(stderr);
            write_out(out);
            if(best==0){ fprintf(stderr,"SOLVED\n"); break; }
        }
        if(it%20000==0){
            fprintf(stderr,"tabu it=%ld viol=%ld best=%ld %.0fs\n",it,total_viol,best,now_s()-t0);
            if(total_viol > best+6){ memcpy(adj,bestadj,sizeof(mask)*n); recount(); }
        }
        /* Large shake: the near-miss seeds all share their first 35 vertices,
         * so without a real perturbation the whole population explores one
         * basin.  Kick K random edges and let the tabu re-descend. */
        if(g_shake > 0 && it % g_shake == 0){
            int K = 4 + (int)(rnd()%16);
            for(int z=0;z<K;z++){
                int u=rndint(n), v=rndint(n);
                if(u==v) continue;
                int ex=(adj[u]>>v&1ULL)?1:0;
                flip(u,v,ex);
            }
        }
    }
    memcpy(adj,bestadj,sizeof(mask)*n); recount();
    write_out(out);
    printf("TABU best viol=%ld (K4=%ld I6=%ld) n=%ld\n",total_viol,total_k4,total_i6,n);
    return (int)total_viol;
}


/* run the tabu move selection for at most `steps` iterations on the current
 * graph; returns the best violation count seen and leaves the best graph in
 * `adj`. */

/* ------------------------------------------------------------------ */
/* "Kill" move: remove one edge of a 4-clique and simultaneously add  */
/* edges that destroy the independent 6-sets this removal creates.     */
/* A single flip cannot work here: deleting a clique edge always      */
/* creates fresh independent 6-sets (the common non-neighbours of the  */
/* endpoints), so both effects have to be paid for in one move.       */
/* ------------------------------------------------------------------ */
static int kill_move(long n){
    int k4[4];
    if(!find_k4(k4)) return 0;
    int e[6][2]={{k4[0],k4[1]},{k4[0],k4[2]},{k4[0],k4[3]},
                  {k4[1],k4[2]},{k4[1],k4[3]},{k4[2],k4[3]}};
    long best=1L<<60; int bx=-1,by=-1,bu=-1,bv=-1;
    for(int q=0;q<6;q++){
        int x=e[q][0], y=e[q][1];
        if(!(adj[x]>>y&1ULL)) continue;
        int s4[64][4]; int ns=enum_ind4(x,y,s4);
        if(ns>48) ns=48;
        for(int r=0;r<ns;r++){
            int cand[6][2]={{x,y},{x,s4[r][0]},{y,s4[r][0]},
                            {s4[r][0],s4[r][1]},{s4[r][0],s4[r][2]},{s4[r][1],s4[r][2]}};
            for(int z=0;z<6;z++){
                int u=cand[z][0], v=cand[z][1];
                if(u==v) continue;
                if(adj[u]>>v&1ULL) continue;
                long before=total_viol;
                flip(x,y,1); flip(u,v,0);
                long after=total_viol;
                flip(u,v,1); flip(x,y,0);
                if(after-before<best){ best=after-before; bx=x; by=y; bu=u; bv=v; }
            }
        }
    }
    if(bx<0) return 0;
    flip(bx,by,1);
    flip(bu,bv,0);
    return 1;
}

static int tabu_refine(long n, long steps){
    long best=total_viol;
    mask bestadj[MAXN]; memcpy(bestadj,adj,sizeof(mask)*n);
    for(long it=1; it<=steps; it++){
        long bi=-1,bj=-1; int bd=1<<30;
        for(int a=0;a<n;a++) for(int b=a+1;b<n;b++){
            int ex=(adj[a]>>b&1ULL)?1:0;
            int d = ex ? (-cnt4(a,b)+cnt6(a,b)) : (+cnt4(a,b)-cnt6(a,b));
            if(d<bd){ bd=d; bi=a; bj=b; }
        }
        if(bi<0) break;
        int ex=(adj[bi]>>bj&1ULL)?1:0;
        flip(bi,bj,ex);
        if(total_viol<best){ best=total_viol; memcpy(bestadj,adj,sizeof(mask)*n);
            if(best==0) break; }
    }
    memcpy(adj,bestadj,sizeof(mask)*n); recount();
    return (int)best;
}


/* ------------------------------------------------------------------ */
/* Memetic search: population + partition crossover + tabu refinement. */
/* The crossover is the GWR-style partition crossover used for Ramsey    */
/* graphs: the vertices are split into blocks and each block-pair's     */
/* edges are inherited from one of the two parents.  Two 12-violation  */
/* graphs can carry their good parts in different places, so the child */
/* can land in a place neither parent could reach alone.                */
/* ------------------------------------------------------------------ */
#define MEMPOP 16
static mask popm[MEMPOP][MAXN];
static long popE[MEMPOP];

static int mem_run(long n, const long *seedv, double budget, uint64_t seed, const char *out){
    rs = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    if(!rs) rs=88172645463325252ULL;
    FULL=((mask)1<<n)-1;
    memcpy(adj, seedv, sizeof(mask)*n);
    /* seed the population: the given graph plus random variants of it */
    for(int i=0;i<MEMPOP;i++){
        memcpy(popm[i], seedv, sizeof(mask)*n);
        int mut = (i==0)?0:(1+(int)(rnd()%6));
        for(int z=0;z<mut;z++){
            int u=rndint(n), v=rndint(n);
            if(u==v) continue;
            popm[i][u]^=(mask)1<<v; popm[i][v]^=(mask)1<<u;
        }
        mask sa[MAXN]; memcpy(sa,popm[i],sizeof(mask)*n);
        memcpy(adj,sa,sizeof(mask)*n); recount();
        /* every population member must be a locally optimised solution --
         * otherwise the crossover just mixes two bad graphs */
        popE[i] = tabu_refine(n, 600);
        memcpy(popm[i], adj, sizeof(mask)*n);
    }
    long best=1L<<60; mask bestadj[MAXN];
    double t0=now_s(); long it=0;
    while(now_s()-t0<budget){
        it++;
        /* tournament selection of two parents */
        int p1=0,p2=0;
        for(int r=0;r<2;r++){ int c=(int)(rnd()%MEMPOP); if(popE[c]<popE[p1]) p1=c; }
        for(int r=0;r<2;r++){ int c=(int)(rnd()%MEMPOP); if(popE[c]<popE[p2]) p2=c; }
        if(p1==p2) p2=(p2+1)%MEMPOP;
        /* GWR-style uniform crossover: for each vertex v (in order) pick one
         * parent at random and inherit v's edges to the already-assigned
         * vertices from it.  Every edge is decided exactly once, so the child
         * stays a consistent graph, and the per-vertex parent choice gives
         * much more diversity than a single row cut. */
        mask child[MAXN];
        for(int v=0;v<n;v++) child[v]=0;
        for(int v=0;v<n;v++){
            int par = (rnd()&1ULL) ? p1 : p2;
            for(int w=0;w<v;w++){
                if(popm[par][w]>>v&1ULL){ child[w]|=(mask)1<<v; child[v]|=(mask)1<<w; }
            }
        }
        /* mutation */
        int mut = (rnd()%3==0)?(1+(int)(rnd()%3)):0;
        for(int z=0;z<mut;z++){
            int u=rndint(n), v=rndint(n);
            if(u==v) continue;
            child[u]^=(mask)1<<v; child[v]^=(mask)1<<u;
        }
        /* tabu refinement of the child */
        memcpy(adj,child,sizeof(mask)*n); recount();
        int childBest = tabu_refine(n, 400);
        /* replace the worst member if the child is better */
        int worst=0; for(int i=1;i<MEMPOP;i++) if(popE[i]>popE[worst]) worst=i;
        if(childBest < popE[worst]){
            memcpy(popm[worst], adj, sizeof(mask)*n);
            popE[worst]=childBest;
        }
        if(childBest < best){
            best=childBest; memcpy(bestadj, adj, sizeof(mask)*n);
            fprintf(stderr,"mem viol=%ld (K4=%ld I6=%ld) it=%ld (%.0fs)\n",best,total_k4,total_i6,it,now_s()-t0);
            fflush(stderr);
            write_out(out);
            if(best==0){ fprintf(stderr,"SOLVED\n"); break; }
        }
        if(it%5000==0) fprintf(stderr,"mem it=%ld best=%ld %.0fs\n",it,best,now_s()-t0);
    }
    memcpy(adj,bestadj,sizeof(mask)*n); recount();
    write_out(out);
    printf("MEM best viol=%ld (K4=%ld I6=%ld) n=%ld\n",total_viol,total_k4,total_i6,n);
    return (int)total_viol;
}

static void write_out(const char *path){
    FILE *f=fopen(path,"w"); if(!f) return;
    fprintf(f,"{\"adjacency\":[");
    for(int i=0;i<n;i++){
        if(i) fprintf(f,",");
        fprintf(f,"\"");
        for(int j=0;j<n;j++) fputc((adj[i]>>j&1ULL)?'1':'0', f);
        fprintf(f,"\"");
    }
    fprintf(f,"]}\n");
    fclose(f);
}

int main(int argc,char**argv){
    if(argc<5){ fprintf(stderr,"usage: p003r <seed.json|random|tabu> <secs> <seed> <out.json> [n]\n"); return 1; }
    const char *seedf=argv[1];
    if(!strcmp(seedf,"mem") || !strcmp(seedf,"tabu")){
        /* p003r tabu <seed.json|random> <secs> <seed> <out.json> [n] */
        double budget=atof(argv[3]);
        uint64_t sd=strtoull(argv[4],NULL,10);
        const char *out=argv[5];
        if(argc<6){ fprintf(stderr,"usage: p003r tabu <seed|random> <secs> <seed> <out> [n]\n"); return 1; }
        rs = sd*6364136223846793005ULL+1442695040888963407ULL; if(!rs) rs=88172645463325252ULL;
        const char *sf=argv[2];
        if(!strcmp(sf,"random")){
            n=(argc>6)?atoi(argv[6]):36;
            FULL=((mask)1<<n)-1;
            for(int i=0;i<n;i++) adj[i]=0;
            for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) if(rnd()&1){ adj[i]|=(mask)1<<j; adj[j]|=(mask)1<<i; }
        } else {
            FILE *f=fopen(sf,"r"); if(!f){perror("open seed");return 1;}
            char buf[400000]; size_t L=fread(buf,1,sizeof(buf)-1,f); buf[L]=0; fclose(f);
            char *pp=strstr(buf,"\"adjacency\""); if(!pp){fprintf(stderr,"no adjacency\n");return 1;}
            pp=strchr(pp,'[');
            char rows[64][64]; int nr=0; char *q=pp+1;
            while(q && nr<64){ char *a=strchr(q,'"'); if(!a)break; char *b=strchr(a+1,'"'); if(!b)break;
                int len=(int)(b-a-1); if(len>63)len=63; memcpy(rows[nr],a+1,len); rows[nr][len]=0; nr++; q=b+1; }
            n=nr; for(int i=0;i<n;i++) adj[i]=0;
            for(int i=0;i<n;i++) for(int j=0;j<(int)strlen(rows[i])&&j<n;j++)
                if(rows[i][j]=='1') adj[i]|=(mask)1<<j;
        }
        if(argc>6) g_tenure_base = atol(argv[6]);
        if(argc>7) g_shake = atol(argv[7]);
        if(argc>8) g_kill = atoi(argv[8]);
        if(!strcmp(seedf,"mem")) mem_run(n, adj, budget, sd, out);
        else tabu_run(n, adj, budget, sd, out);
        return 0;
    }
    double budget=atof(argv[2]);
    rs = strtoull(argv[3],NULL,10) * 6364136223846793005ULL + 1442695040888963407ULL;
    if(!rs) rs=88172645463325252ULL;
    const char *out=argv[4];

    if(!strcmp(seedf,"random")){
        n = (argc>5)?atoi(argv[5]):36;
        FULL = (n==64)?~0ULL:((mask)1<<n)-1;
        for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) if(rnd()&1){ adj[i]|=(mask)1<<j; adj[j]|=(mask)1<<i; }
    } else {
        FILE *f=fopen(seedf,"r"); if(!f){perror("open seed");return 1;}
        char buf[400000]; size_t L=fread(buf,1,sizeof(buf)-1,f); buf[L]=0; fclose(f);
        char *p=strstr(buf,"\"adjacency\"");
        if(!p){ fprintf(stderr,"no adjacency key\n"); return 1; }
        p=strchr(p,'[');
        /* collect every quoted string after the '[' : those are the rows */
        char rows[64][64]; int nr=0;
        char *q=p+1;
        while(q && nr<64){
            char *a=strchr(q,'"'); if(!a) break;
            char *b=strchr(a+1,'"'); if(!b) break;
            int len=(int)(b-a-1);
            if(len>63) len=63;
            memcpy(rows[nr],a+1,len); rows[nr][len]=0; nr++;
            q=b+1;
        }
        n=nr;
        for(int i=0;i<n;i++) adj[i]=0;
        for(int i=0;i<n;i++)
            for(int j=0;j<(int)strlen(rows[i]) && j<n;j++)
                if(rows[i][j]=='1') adj[i]|=(mask)1<<j;
        fprintf(stderr,"seed loaded: n=%d\n",n);
        FULL=(mask)1<<n; FULL=((mask)1<<n)-1;
        fprintf(stderr,"seed loaded: n=%d\n",n);
    }
    recount();
    fprintf(stderr,"start: viol=%ld (K4=%ld, I6=%ld)\n",total_viol,total_k4,total_i6);
    long best=total_viol; mask bestadj[MAXN]; memcpy(bestadj,adj,sizeof(mask)*n);

    double t0=now_s(); long it=0; long bestit=0; double T=1.0;
    while(now_s()-t0<budget){
        it++;
        /* ---- compound move (ejection chain) -------------------------------
         * Removing one edge of a 4-clique can create several independent
         * 6-sets.  A single flip cannot repair that in one step, so we do
         * the removal and the repair of one resulting independent 6-set as
         * ONE move and evaluate the pair together.                        */
        int a=-1,b=-1,exists=0;
        if((rnd()&3ULL)!=0){
            int k4[4];
            if(find_k4(k4)){
                int e[6][2]={{k4[0],k4[1]},{k4[0],k4[2]},{k4[0],k4[3]},
                              {k4[1],k4[2]},{k4[1],k4[3]},{k4[2],k4[3]}};
                /* best edge to remove = most negative delta */
                int bi=0; int bd=1<<30;
                for(int q=0;q<6;q++){
                    int x=e[q][0], y=e[q][1];
                    int d = -cnt4(x,y) + cnt6(x,y);
                    if(d<bd){ bd=d; bi=q; }
                }
                int x=e[bi][0], y=e[bi][1];
                long before=total_viol;
                flip(x,y,1);                       /* remove the edge */
                /* the independent 6-sets that appeared contain x and y */
                int s4[64][4]; int ns = enum_ind4(x,y,s4);
                int bestp=-1,bestq=-1,bestd=1<<30;
                for(int q=0;q<ns;q++){
                    int c6[6][2]={{x,y},{x,s4[q][0]},{y,s4[q][0]},
                                  {s4[q][0],s4[q][1]},{s4[q][0],s4[q][2]},{s4[q][1],s4[q][2]}};
                    for(int r=0;r<6;r++){
                        int p=c6[r][0], qq=c6[r][1];
                        if(p==qq) continue;
                        if(adj[p]>>qq&1ULL) continue;   /* already present */
                        int d = cnt4(p,qq) - cnt6(p,qq);
                        if(d<bestd){ bestd=d; bestp=p; bestq=qq; }
                    }
                }
                if(bestp>=0 && bestd<0){
                    flip(bestp,bestq,0);             /* add the repairing edge */
                    a=-1; b=-1;                      /* compound move applied */
                } else {
                    flip(x,y,0);                    /* undo: keep only the removal */
                    a=x; b=y; exists=0;             /* (handled as a plain flip below) */
                }
                long after=total_viol;
                int dd = (int)(after-before);
                int ok2 = (dd<=0);
                if(!ok2){
                    double u2 = (double)(rnd()>>11)/9007199254740992.0;
                    if(u2 < exp(-(double)dd/T)) { /* keep */ }
                    else { if(bestp>=0){ flip(bestp,bestq,1); } flip(x,y,0); }
                }
                if(a>=0){
                    /* the plain-flip path was taken */
                    long b2=total_viol;
                    flip(a,b,0);
                    int d2=(int)(total_viol-b2);
                    int ok3=(d2<=0);
                    if(!ok3){
                        double u3=(double)(rnd()>>11)/9007199254740992.0;
                        if(!(u3 < exp(-(double)d2/T))) flip(a,b,1);
                    }
                }
                {
                    long EP=200000; long ph=it%EP;
                    T = 1.0*pow(0.03,(double)ph/(double)EP);
                    if(ph==0){ memcpy(adj,bestadj,sizeof(mask)*n); recount();
                        int kk=2+rndint(6);
                        for(int z=0;z<kk;z++){ int u3=rndint(n),v3=rndint(n); if(u3==v3)continue;
                            int ex=(adj[u3]>>v3&1ULL)?1:0; flip(u3,v3,ex); } }
                }
                if(total_viol<best){ best=total_viol; memcpy(bestadj,adj,sizeof(mask)*n);
                    fprintf(stderr,"viol=%ld (K4=%ld I6=%ld) at iter %ld (%.0fs)\n",best,total_k4,total_i6,it,now_s()-t0);
                    write_out(out); if(best==0){ fprintf(stderr,"SOLVED\n"); break; } }
                continue;
            }
        }
        /* ---- fallback: single targeted flip ---- */
        if((rnd()&1ULL)==0){
            /* find a K4: a vertex a, then a triangle inside N(a) */
            int u=rndint(n);
            mask N=adj[u]; int nb[40], m=0; mask t=N;
            while(t){ nb[m++]=__builtin_ctzll(t); t&=t-1; }
            int found=0;
            for(int i=0;i<m && !found;i++) for(int j=i+1;j<m && !found;j++){
                if(!(adj[nb[i]]>>nb[j]&1ULL)) continue;
                for(int k=j+1;k<m && !found;k++){
                    if(!(adj[nb[i]]>>nb[k]&1ULL) || !(adj[nb[j]]>>nb[k]&1ULL)) continue;
                    /* K4 = {u, nb[i], nb[j], nb[k]} */
                    int e[6][2]={{u,nb[i]},{u,nb[j]},{u,nb[k]},{nb[i],nb[j]},{nb[i],nb[k]},{nb[j],nb[k]}};
                    int pick=rndint(6);
                    a=e[pick][0]; b=e[pick][1];
                    found=1;
                }
            }
            if(!found){ a=rndint(n); b=rndint(n); }
        } else {
            /* find an independent 6-set: an existing edge (u,v) plus an
             * independent 4-set inside the common non-neighbours */
            int u=rndint(n);
            mask N=adj[u]; int nb[40], m=0; mask t=N;
            while(t){ nb[m++]=__builtin_ctzll(t); t&=t-1; }
            int found=0;
            for(int i=0;i<m && !found;i++){
                int v=nb[i];
                mask S = FULL & ~(adj[u]|adj[v]|((mask)1<<u)|((mask)1<<v));
                int sb[40], sm=0; mask t2=S;
                while(t2){ sb[sm++]=__builtin_ctzll(t2); t2&=t2-1; }
                for(int i1=0;i1<sm && !found;i1++) for(int j1=i1+1;j1<sm && !found;j1++){
                    if(adj[sb[i1]]>>sb[j1]&1ULL) continue;
                    for(int k1=j1+1;k1<sm && !found;k1++){
                        if(adj[sb[i1]]>>sb[k1]&1ULL || adj[sb[j1]]>>sb[k1]&1ULL) continue;
                        for(int l1=k1+1;l1<sm && !found;l1++){
                            if(adj[sb[i1]]>>sb[l1]&1ULL || adj[sb[j1]]>>sb[l1]&1ULL || adj[sb[k1]]>>sb[l1]&1ULL) continue;
                            /* independent 6 = {u,v,sb..} : add a missing edge among them */
                            int cand[6][2]={{u,v},{u,sb[i1]},{v,sb[i1]},{sb[i1],sb[j1]},{sb[i1],sb[k1]},{sb[j1],sb[k1]}};
                            int pick=rndint(6);
                            a=cand[pick][0]; b=cand[pick][1];
                            found=1;
                        }
                    }
                }
            }
            if(!found){ a=rndint(n); b=rndint(n); }
        }
        if(a==b) continue;
        if(a>b){int t=a;a=b;b=t;}
        exists = (adj[a]>>b&1ULL)?1:0;
        long before=total_viol;
        int d = exists ? (-cnt4(a,b)+cnt6(a,b)) : (+cnt4(a,b)-cnt6(a,b));
        flip(a,b,exists);
        /* simulated annealing on the violation count */
        int accept;
        if (d <= 0) accept = 1;
        else {
            double u = (double)(rnd()>>11)/9007199254740992.0;   /* [0,1) */
            accept = (u < exp(-(double)d/T));
        }
        if(!accept){ flip(a,b,!exists); }
        /* epoch schedule: anneal from T0 down to T1 inside each epoch, then
         * restart from the best-so-far.  Classic iterated local search. */
        {
            long EP = 200000;
            long ph = it % EP;
            T = 1.0 * pow(0.03, (double)ph/(double)EP);
            if (ph==0){
                /* ILS kick: restart from the best solution and perturb it
                 * with a few random edge flips, then anneal again. */
                memcpy(adj,bestadj,sizeof(mask)*n); recount();
                int kk = 2 + rndint(6);
                for(int z=0; z<kk; z++){
                    int u=rndint(n), v=rndint(n);
                    if(u==v) continue;
                    int ex = (adj[u]>>v&1ULL)?1:0;
                    flip(u,v,ex);
                }
            }
        }
        if(total_viol<best){
            best=total_viol; memcpy(bestadj,adj,sizeof(mask)*n); bestit=it;
            fprintf(stderr,"viol=%ld (K4=%ld I6=%ld) at iter %ld (%.0fs)\n",best,total_k4,total_i6,it,now_s()-t0);
            write_out(out);
            if(best==0){ fprintf(stderr,"SOLVED\n"); break; }
        }
        if(it%200000==0) fprintf(stderr,"iter %ld viol=%ld best=%ld %.0fs\n",it,total_viol,best,now_s()-t0);
        if(it%20000==0){                      /* self-check: incremental vs full recount */
            mask save[MAXN]; memcpy(save,adj,sizeof(mask)*n);
            long inc=total_viol; recount();
            if(inc!=total_viol){
                fprintf(stderr,"MISMATCH at iter %ld: incremental=%ld recount=%ld (K4 %ld/%ld I6 %ld/%ld)\n",
                        it,inc,total_viol,total_k4,total_i6,inc,total_viol);
                memcpy(adj,save,sizeof(mask)*n); recount();
            }
        }
    }
    memcpy(adj,bestadj,sizeof(mask)*n);
    recount();
    write_out(out);
    printf("BEST viol=%ld (K4=%ld I6=%ld) n=%d\n",total_viol,total_k4,total_i6,n);
    return 0;
}
