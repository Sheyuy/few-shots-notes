// Search for a (4,6)-graph on 36 vertices.  Two modes:
//   B: keep the 35-vertex graph fixed, choose the neighbourhood S of vertex 36.
//   A: simulate annealing over all edges of a 36-vertex graph.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

static int N;
static uint64_t adj[64], ABOVE[64], MASK;

static uint64_t rng_state = 88172645463325252ULL;
static inline uint64_t rnd64(void){ rng_state^=rng_state<<13; rng_state^=rng_state>>7; rng_state^=rng_state<<17; return rng_state; }
static inline double rnd(void){ return (double)(rnd64()>>11) * (1.0/9007199254740992.0); }
static inline int rndint(int n){ return (int)(rnd64()%(uint64_t)n); }
static inline int ctz(uint64_t x){ return __builtin_ctzll(x); }
static inline int pc(uint64_t x){ return __builtin_popcountll(x); }

static double now(void){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); return ts.tv_sec+1e-9*ts.tv_nsec; }

static int load_graph(const char *path){
    FILE *f=fopen(path,"r"); if(!f){perror(path);exit(1);} 
    if(fscanf(f,"%d",&N)!=1){exit(1);} 
    char line[256];
    for(int i=0;i<N;i++){
        if(fscanf(f,"%255s",line)!=1){fprintf(stderr,"bad file\n");exit(1);} 
        if((int)strlen(line)!=N){fprintf(stderr,"bad line %d\n",i);exit(1);} 
        adj[i]=0;
        for(int j=0;j<N;j++) if(line[j]=='1') adj[i]|=1ULL<<j;
    }
    fclose(f);
    for(int i=0;i<N;i++){ ABOVE[i]=~((1ULL<<(i+1))-1); }
    MASK=(1ULL<<N)-1;
    return N;
}

// number of independent 4-subsets of W
static int count4(uint64_t W){
    int total=0;
    uint64_t m1=W;
    while(m1){
        int a=ctz(m1); m1&=m1-1;
        uint64_t W1=W & ~adj[a] & ABOVE[a];
        uint64_t m2=W1;
        while(m2){
            int b=ctz(m2); m2&=m2-1;
            uint64_t W2=W1 & ~adj[b] & ABOVE[b];
            uint64_t m3=W2;
            while(m3){
                int c=ctz(m3); m3&=m3-1;
                total += pc(W2 & ~adj[c] & ABOVE[c]);
            }
        }
    }
    return total;
}

// edges induced by A
static int edges_inside(uint64_t A){
    int s=0; uint64_t xm=A;
    while(xm){ int x=ctz(xm); xm&=xm-1; s += pc(adj[x]&A); }
    return s/2;
}

// triangles inside S
static int triangles(uint64_t S){
    int t=0; uint64_t xm=S;
    while(xm){
        int x=ctz(xm); xm&=xm-1;
        uint64_t A = S & adj[x] & ABOVE[x];
        uint64_t ym=A;
        while(ym){ int y=ctz(ym); ym&=ym-1; t += pc(A & adj[y] & ABOVE[y]); }
    }
    return t;
}

// independent 5-subsets of C
static int indep5(uint64_t C){
    int total=0;
    uint64_t m1=C;
    while(m1){
        int a=ctz(m1); m1&=m1-1;
        uint64_t W1=C & ~adj[a] & ABOVE[a];
        uint64_t m2=W1;
        while(m2){
            int b=ctz(m2); m2&=m2-1;
            uint64_t W2=W1 & ~adj[b] & ABOVE[b];
            uint64_t m3=W2;
            while(m3){
                int c=ctz(m3); m3&=m3-1;
                uint64_t W3=W2 & ~adj[c] & ABOVE[c];
                uint64_t m4=W3;
                while(m4){
                    int d=ctz(m4); m4&=m4-1;
                    total += pc(W3 & ~adj[d] & ABOVE[d]);
                }
            }
        }
    }
    return total;
}

// full counts for a 36-vertex graph
static void full_counts(long *k4, long *i6){
    long a=0,b=0;
    for(int x=0;x<N;x++){
        uint64_t A=MASK & ~adj[x] & ~(1ULL<<x);
        for(int y=x+1;y<N;y++){
            if(adj[x]>>y&1){ // edge: count K4 with x<y as two smallest
                uint64_t C=adj[x]&adj[y]&ABOVE[y];
                uint64_t cm=C;
                while(cm){ int z=ctz(cm); cm&=cm-1; a += pc(adj[z]&C&ABOVE[z]); }
                // independent 4-subsets of W with x<y nonedge not applicable
            } else {
                uint64_t W=MASK&~adj[x]&~adj[y]&ABOVE[y]&~(1ULL<<y);
                b += count4(W);
            }
        }
        (void)A;
    }
    *k4=a; *i6=b;
}

static void delta_flip(int u,int v,long *dk4,long *di6){
    uint64_t bu=1ULL<<u, bv=1ULL<<v;
    uint64_t C=adj[u]&adj[v];
    long e=edges_inside(C);
    uint64_t W=MASK & ~(adj[u]|adj[v]) & ~bu & ~bv;
    long c4=count4(W);
    if((adj[u]>>v)&1){ *dk4=-e; *di6=+c4; }
    else { *dk4=+e; *di6=-c4; }
}

static void print_graph(const char *path){
    FILE *f=fopen(path,"w");
    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++) fputc((adj[i]>>j&1)?'1':'0',f);
        fputc('\n',f);
    }
    fclose(f);
}

// ---------- mode B ----------
static int run_mode_b(int seed, double budget, int verbose){
    load_graph("seeds/g0.txt"); // placeholder, reloaded by caller is not done here
    return 0;
}

int main(int argc,char**argv){
    if(argc<2){fprintf(stderr,"usage: %s B|A file budget_sec\n",argv[0]);return 1;}
    char mode=argv[1][0];
    const char *path=argv[2];
    double budget=argc>3?atof(argv[3]):60.0;
    double t0=now();
    rng_state ^= (uint64_t)time(NULL)*2654435761u;



    if(mode=='M'){
        load_graph(path);
        int n=N;
        uint64_t S=0;
        for(int i=0;i<n;i++) if(rnd()<0.4) S|=1ULL<<i;
        long p=triangles(S)+indep5(MASK&~S);
        long best=p;
        long tabu2[64]; for(int i=0;i<64;i++) tabu2[i]=-1;
        long it=0;
        while(now()-t0<budget){
            it++;
            if(p==0) break;
            // pick a violated constraint and one of its vertices
            int cand[8]; int nc=0; int kind=0; // 0 triangle, 1 I5
            // find a triangle in S
            uint64_t Sm=S;
            for(int tries=0;tries<50 && nc==0;tries++){
                if(!Sm) break;
                int u=ctz(Sm); Sm&=Sm-1;
                uint64_t A=S&adj[u];
                if(pc(A)<2) continue;
                // find v,w in A adjacent
                uint64_t vm=A;
                while(vm && nc==0){
                    int v=ctz(vm); vm&=vm-1;
                    uint64_t W=A&adj[v];
                    if(W){ int w=ctz(W); cand[0]=u;cand[1]=v;cand[2]=w;nc=3;kind=0; }
                }
            }
            // find an independent 5-set in complement
            if(nc==0){
                uint64_t Cm=MASK&~S;
                for(int tries=0;tries<200 && nc==0;tries++){
                    uint64_t rest=Cm; int sel[5]; int k=0;
                    while(k<5 && rest){
                        uint64_t r=rest; int cnt=pc(r);
                        int pick=rndint(cnt); int v=-1;
                        while(pick>=0){ v=ctz(r); r&=r-1; pick--; }
                        sel[k++]=v; rest = rest & ~adj[v] & ABOVE[v];
                    }
                    if(k==5){ for(int i=0;i<5;i++) cand[i]=sel[i]; nc=5; kind=1; }
                }
            }
            if(nc==0){ // fallback random flip
                int v=rndint(n); cand[0]=v; nc=1; kind=0;
            }
            // choose best vertex among cand
            int bv=-1; long bp=1L<<60; int bd=0;
            for(int ci=0;ci<nc;ci++){
                int v=cand[ci];
                if(tabu2[v]>it) continue;
                uint64_t bvm=1ULL<<v;
                int inS=(S>>v)&1;
                uint64_t A=S&adj[v];
                int dt=edges_inside(A);
                int di;
                if(inS){ uint64_t W=(MASK&~S)&~adj[v]&~bvm; di=+count4(W); }
                else { uint64_t W=(MASK&~S)&~adj[v]&~bvm; di=-count4(W); }
                long np=p+(inS?-dt:+dt)+di;
                if(np<bp || (np==bp && rnd()<0.5)){ bp=np; bv=v; bd=(inS?1:0); }
            }
            if(bv<0){ // all tabu, random
                bv=cand[rndint(nc)]; 
                int inS=(S>>bv)&1; uint64_t A=S&adj[bv]; int dt=edges_inside(A); int di;
                uint64_t bvm=1ULL<<bv;
                if(inS){ uint64_t W=(MASK&~S)&~adj[bv]&~bvm; di=+count4(W);} else {uint64_t W=(MASK&~S)&~adj[bv]&~bvm; di=-count4(W);}
                bp=p+(inS?-dt:+dt)+di; bd=inS;
            }
            S^=1ULL<<bv; p=bp; tabu2[bv]=it+5+rndint(5);
            if(p<best){ best=p; fprintf(stderr,"t=%.1f best=%ld it=%ld\n",now()-t0,best,it); }
        }
        if(p==0){
            N=n+1; MASK=(1ULL<<N)-1;
            for(int i=0;i<N;i++) ABOVE[i]=~((1ULL<<(i+1))-1); ABOVE[0]=0;
            adj[n]=S; for(int i=0;i<n;i++) if((S>>i)&1) adj[i]|=1ULL<<n;
            long kk4,ii6; full_counts(&kk4,&ii6);
            if(kk4==0&&ii6==0){ print_graph("found_m.txt"); printf("FOUND 36\n"); return 0; }
            else fprintf(stderr,"false zero k4=%ld i6=%ld\n",kk4,ii6);
        }
        fprintf(stderr,"M done best=%ld it=%ld\n",best,it);
        return 2;
    }
    if(mode=='T'){
        load_graph(path);
        int bad=0;
        for(int trial=0;trial<200000;trial++){
            uint64_t S=0;
            for(int i=0;i<N;i++) if(rnd()<0.4) S|=1ULL<<i;
            long p0=triangles(S)+indep5(MASK&~S);
            long true0=triangles(S)+indep5(MASK&~S);
            int v=(int)(rnd64()%N); uint64_t bv=1ULL<<v; int inS=(S>>v)&1;
            uint64_t A=S&adj[v]; int dt=edges_inside(A); int di;
            if(inS){ uint64_t W=(MASK&~S)&~adj[v]&~bv; di=+count4(W); }
            else { uint64_t W=(MASK&~S)&~adj[v]&~bv; di=-count4(W); }
            uint64_t S2=S^bv;
            long p2=triangles(S2)+indep5(MASK&~S2);
            long pred=p0+(inS?-dt:+dt)+di;
            if(pred!=p2){ bad++; if(bad<5) fprintf(stderr,"T mismatch v=%d inS=%d p0=%ld true0=%ld dt=%d di=%d pred=%ld p2=%ld\n",v,inS,p0,true0,dt,di,pred,p2); }
        }
        fprintf(stderr,"T bad=%d\n",bad);
        return 0;
    }
    if(mode=='B'){
        load_graph(path);
        // N is 35, we search S subset of the 35 vertices
        int n=N;
        uint64_t S = 0;
        // random initial S at density ~0.4
        for(int i=0;i<n;i++) if(rnd()<0.4) S|=1ULL<<i;
        long p = triangles(S) + indep5(MASK & ~S);
        long best=p;
        double t0=now();
        double T=2.0, Tmin=1e-3;
        long it=0;
        // cooling: geometric per move until budget
        while(now()-t0<budget){
            int v=(int)(rnd64()%n);
            uint64_t bv=1ULL<<v;
            int inS=(S>>v)&1;
            uint64_t A = S & adj[v];           // neighbours of v in S (v not in adj[v])
            int dt = edges_inside(A);
            int di;
            if(inS){ // remove v: v joins complement
                uint64_t W = (MASK & ~S) & ~adj[v];   // includes v itself
                // exclude v: v is nonadjacent to itself
                W &= ~bv;
                di = +count4(W);
            } else { // add v: v leaves complement
                uint64_t W = (MASK & ~S) & ~adj[v] & ~bv;
                di = -count4(W);
            }
            long np = p + (inS? -dt : +dt) + di;
            if(np<=p || rnd()<exp((double)(p-np)/T)){
                S ^= bv; p=np;
                if(p<best){ best=p; }
                if(p==0){
                    // build graph and verify exactly before declaring
                    N=n+1; MASK=(1ULL<<N)-1;
                    for(int i=0;i<N;i++) ABOVE[i]=~((1ULL<<(i+1))-1);
                    ABOVE[0]=0;
                    adj[n]=S;
                    for(int i=0;i<n;i++) if((S>>i)&1) adj[i]|=1ULL<<n;
                    long kk4,ii6; full_counts(&kk4,&ii6);
                    if(kk4==0 && ii6==0){
                        fprintf(stderr,"modeB FOUND it=%ld\n",it);
                        print_graph("found_b.txt");
                        printf("FOUND 36 vertices\n");
                        return 0;
                    } else {
                        fprintf(stderr,"modeB false zero p=%ld true k4=%ld i6=%ld it=%ld\n",p,kk4,ii6,it);
                        // restore N=35 state is awkward; abort this seed
                        return 3;
                    }
                }
            }
            it++;
            double frac=(now()-t0)/budget;
            if((it & 0xFFFFF)==0) fprintf(stderr,"t=%.1f best=%ld p=%ld T=%.3f\n",now()-t0,best,p,T);
            T = 2.0 * pow(1e-4/2.0, frac);
        }
        fprintf(stderr,"modeB best=%ld it=%ld\n",best,it);
        return 2;
    }

    if(mode=='A'){
        // path is a 35-vertex seed; add vertex 35
        load_graph(path);
        int n=N; // 35
        N=n+1; MASK=(1ULL<<N)-1;
        ABOVE[0]=0; for(int i=0;i<N;i++) ABOVE[i]=~((1ULL<<(i+1))-1);
        adj[n]=0;
        for(int i=0;i<n;i++) if(rnd()<0.45){ adj[n]|=1ULL<<i; adj[i]|=1ULL<<n; }
        long k4,i6; full_counts(&k4,&i6);
        long p=k4+i6, best=p;
        double t0=now(), T=4.0;
        long it=0;
        while(now()-t0<budget){
            int u=(int)(rnd64()%N); int v=(int)(rnd64()%N);
            if(u==v) continue;
            long dk4,di6; delta_flip(u,v,&dk4,&di6);
            long np=p+dk4+di6;
            if(np<=p || rnd()<exp((double)(p-np)/T)){
                uint64_t bu=1ULL<<u,bv=1ULL<<v;
                adj[u]^=bv; adj[v]^=bu;
                p=np;
                if(p<best){ best=p; int vv=1; if(vv) fprintf(stderr,"it=%ld p=%ld k4=%ld i6=%ld\n",it,p,k4,i6); }
                if(p==0){
                    print_graph("found_a.txt");
                    printf("FOUND 36 vertices\n");
                    return 0;
                }
            }
            it++;
            double frac=(now()-t0)/budget;
            if((it & 0xFFFFF)==0) fprintf(stderr,"t=%.1f best=%ld p=%ld T=%.3f\n",now()-t0,best,p,T);
            T = 4.0 * pow(1e-3/4.0, frac);
        }
        fprintf(stderr,"modeA best=%ld it=%ld\n",best,it);
        return 2;
    }
    return 1;
}