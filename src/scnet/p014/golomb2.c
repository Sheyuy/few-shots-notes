// golomb2.c : tabu search on +-1 mark shifts with plateau moves, fixed length L
// cost = number of colliding difference pairs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define MAXN 64
#define MAXL 4096
static int N,L,m[MAXN],occ[MAXL+1];
static short cnt[MAXL+1];
static long long cost;
static int tabuMark[MAXN],tabuPos[MAXN];
static unsigned long long rs=88172645463325252ULL;
static inline unsigned long long rnd(){rs^=rs<<13;rs^=rs>>7;rs^=rs<<17;return rs;}
static long long now_ms(){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec*1000LL+t.tv_nsec/1000000;}
static void build(){memset(cnt,0,sizeof(short)*(L+1));cost=0;
  for(int i=0;i<N;i++)for(int j=i+1;j<N;j++){int d=m[j]-m[i];cost+=cnt[d];cnt[d]++;}}
static long long delta(int k,int p){
  int ov[MAXN],nv[MAXN];
  for(int i=0;i<N;i++){ if(i==k){ov[i]=nv[i]=-1;continue;}
    ov[i]=m[k]>m[i]?m[k]-m[i]:m[i]-m[k];
    nv[i]=p   >m[i]?p   -m[i]:m[i]-p; }
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
    nv[i]=p   >m[i]?p   -m[i]:m[i]-p; }
  for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; cnt[ov[i]]--; }
  for(int i=0;i<N;i++){ if(i==k||ov[i]==nv[i])continue; cnt[nv[i]]++; }
  occ[m[k]]=0; occ[p]=1; m[k]=p;
}
int main(int argc,char**argv){
  L=argc>1?atoi(argv[1]):622; int secs=argc>2?atoi(argv[2]):60;
  rs=argc>3?strtoull(argv[3],0,10):12345;
  N=29;
  int base[29]={0,3,28,33,41,95,156,158,193,207,233,252,267,338,339,355,394,403,421,473,479,502,523,570,580,592,612,616,623};
  for(int i=0;i<N;i++)m[i]=base[i];
  if(L!=623){
    for(int i=0;i<N;i++){long long v=(long long)base[i]*L/623; if(v>L)v=L; m[i]=(int)v;}
    for(int i=1;i<N;i++) if(m[i]<=m[i-1]) m[i]=m[i-1]+1;
    if(m[N-1]!=L){m[N-1]=L; for(int i=N-2;i>=0;i--) if(m[i]>=m[i+1]) m[i]=m[i+1]-1;}
    if(m[0]!=0) for(int i=0;i<N;i++) m[i]-=m[0];
  }
  memset(occ,0,sizeof(int)*(L+1)); for(int i=0;i<N;i++)occ[m[i]]=1;
  build();
  printf("L=%d start=%lld\n",L,cost); fflush(stdout);
  long long best=cost; int bestm[MAXN]; memcpy(bestm,m,sizeof(m));
  long long t0=now_ms(),tend=t0+1000LL*secs,it=0;
  int tenure=7;
  while(now_ms()<tend && best>0){
    int bk=-1,bp=-1; long long bd=1LL<<62; int ties=0;
    for(int k=1;k<N-1;k++){
      for(int sgn=-1;sgn<=1;sgn+=2){
        int p=m[k]+sgn;
        if(p<1||p>L-1||occ[p])continue;
        long long d=delta(k,p);
        int isTabu=(tabuMark[k]==1 && tabuPos[k]==p && it<(long long)tabuMark[k]*0+tabuPos[k]);
        (void)isTabu;
        if(d<bd){bd=d;bk=k;bp=p;ties=1;}
        else if(d==bd){ties++; if(rnd()%(unsigned)ties==0){bk=k;bp=p;}}
      }
    }
    if(bk<0) break;
    apply(bk,bp); cost+=bd; it++;
    if(cost<best){best=cost;memcpy(bestm,m,sizeof(m));
      printf("it=%lld cost=%lld\n",it,best); fflush(stdout);
      if(best==0)break;}
    if((it%500000)==0){printf("  it=%lld cost=%lld best=%lld\n",it,cost,best);fflush(stdout);}
  }
  printf("DONE L=%d best=%lld iters=%lld %.1fs\n",L,best,it,(now_ms()-t0)/1000.0);
  if(best==0){printf("MARKS");for(int i=0;i<N;i++)printf(" %d",bestm[i]);printf("\n");}
  return 0;
}
