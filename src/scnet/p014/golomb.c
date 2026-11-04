// golomb.c : tabu search for golomb rulers with N marks and fixed length L
// cost = number of colliding difference PAIRS (0 => valid ruler)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#define MAXN 64
#define MAXL 4096
static int N,L;
static int m[MAXN];
static short cnt[MAXL+1];
static long long cost;
static int tabu[MAXN][MAXL+1];
static unsigned long long rs=88172645463325252ULL;
static inline unsigned long long rnd(){rs^=rs<<13;rs^=rs>>7;rs^=rs<<17;return rs;}
static long long now_ms(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1000LL+t.tv_nsec/1000000;}
static void build(){
    memset(cnt,0,sizeof(short)*(L+1)); cost=0;
    for(int i=0;i<N;i++)for(int j=i+1;j<N;j++){int d=m[j]-m[i];cost+=cnt[d];cnt[d]++;}
}
// exact delta: remove all old differences, then add all new ones, then undo
static long long delta(int k,int p){
    static int ov[MAXN],nv[MAXN];
    for(int i=0;i<N;i++){ if(i==k){ov[i]=nv[i]=-1;continue;}
        ov[i]=m[k]>m[i]?m[k]-m[i]:m[i]-m[k];
        nv[i]=p   >m[i]?p   -m[i]:m[i]-p;
    }
    long long dc=0;
    for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; dc-=(long long)(cnt[ov[i]]-1); cnt[ov[i]]--; }
    for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; dc+=(long long)cnt[nv[i]];        cnt[nv[i]]++; }
    for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; cnt[nv[i]]--; }
    for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; cnt[ov[i]]++; }
    return dc;
}
static void apply(int k,int p){
    int ov[MAXN],nv[MAXN];
    for(int i=0;i<N;i++){ if(i==k){ov[i]=nv[i]=-1;continue;}
        ov[i]=m[k]>m[i]?m[k]-m[i]:m[i]-m[k];
        nv[i]=p   >m[i]?p   -m[i]:m[i]-p;
    }
    for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; cnt[ov[i]]--; }
    for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; cnt[nv[i]]++; }
    m[k]=p;
}
int main(int argc,char**argv){
    L=argc>1?atoi(argv[1]):622; int secs=argc>2?atoi(argv[2]):60; rs=argc>3?strtoull(argv[3],0,10):88172645463325252ULL;
    // baseline Singer ruler (length 623)
    int base[29]={0,3,28,33,41,95,156,158,193,207,233,252,267,338,339,355,394,403,421,473,479,502,523,570,580,592,612,616,623};
    N=29;
    for(int i=0;i<N;i++) m[i]=base[i];
    if(argc>4 && argv[4][0]=='1'){ // random start: pick N distinct marks in [0,L] with 0 and L fixed
        int occ0[MAXL+1]; memset(occ0,0,sizeof(int)*(L+1)); occ0[0]=occ0[L]=1;
        for(int i=1;i<N-1;i++){int p2;do{p2=1+(int)(rnd()%(unsigned)(L-1));}while(occ0[p2]);occ0[p2]=1;m[i]=p2;}
        m[0]=0;m[N-1]=L;
        for(int i=1;i<N;i++)for(int j=i;j>0&&m[j]<m[j-1];j--){int x=m[j];m[j]=m[j-1];m[j-1]=x;}
    } else if(L!=623){ // compress onto [0,L]
        for(int i=0;i<N;i++){ long long v=(long long)base[i]*L/623; if(v>L)v=L; m[i]=(int)v; }
        for(int i=1;i<N;i++) if(m[i]<=m[i-1]) m[i]=m[i-1]+1;
        if(m[N-1]!=L){ m[N-1]=L; for(int i=N-2;i>=0;i--) if(m[i]>=m[i+1]) m[i]=m[i+1]-1; }
        if(m[0]!=0){ for(int i=0;i<N;i++) m[i]-=m[0]; }
    }
    build();
    printf("L=%d start cost=%lld marks:",L,cost); for(int i=0;i<N;i++)printf(" %d",m[i]); printf("\n"); fflush(stdout);
    long long best=cost; int bestm[MAXN]; memcpy(bestm,m,sizeof(m));
    long long t0=now_ms(), tend=t0+1000LL*secs; long long it=0;
    double T=(argc>5)?atof(argv[5]):3.0; double T0=T;
    int occ[MAXL+1]; memset(occ,0,sizeof(int)*(L+1)); for(int i=0;i<N;i++)occ[m[i]]=1;
    while(now_ms()<tend && best>0){
        // random mark and random free target
        int k=1+(int)(rnd()%(N-2));
        int p=1+(int)(rnd()%(unsigned)(L-1));
        if(occ[p]) continue;
        long long d=delta(k,p);
        if(d<=0 || (double)(rnd()%1000000)/1000000.0 < exp(-(double)d/T)){
            int old=m[k];
            apply(k,p);
            cost+=d;
            occ[old]=0; occ[p]=1;
            it++;
            if(cost<best){ best=cost; memcpy(bestm,m,sizeof(m));
                if(best==0){ printf("FOUND it=%lld\n",it); break; }
            }
            if((it%2000000)==0){ printf("  it=%lld cost=%lld best=%lld T=%.2f\n",it,cost,best,T); fflush(stdout); }
        }
        if((it%2000000)==0 && it>0){ T*=0.9; if(T<0.25)T=T0; }
    }
    printf("DONE L=%d best=%lld iters=%lld time=%.1fs\n",L,best,it,(now_ms()-t0)/1000.0);
    if(best==0){ printf("MARKS"); for(int i=0;i<N;i++)printf(" %d",bestm[i]); printf("\n"); }
    return best==0?0:1;
}
