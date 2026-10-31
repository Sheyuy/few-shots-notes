#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define N 6561
#define MAXS 900
static uint8_t ADDHI[81][81],ADDLO[81][81],NEGHI[81],NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N],tabu[N]; static int conf[N]; static int sz;
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static void remp(int p){int pos=-1;for(int i=0;i<sz;i++)if(S[i]==p){pos=i;break;}S[pos]=S[--sz];inS[p]=0;for(int i=0;i<sz;i++)conf[third(p,S[i])]--;}
static uint64_t rng=0x123456789abcdefULL;
static inline uint64_t rnd(void){uint64_t x=rng;x^=x<<13;x^=x>>7;x^=x<<17;rng=x;return x;}
static inline int rndint(int n){return (int)(rnd()%(uint64_t)n);}
static uint16_t base[MAXS]; static int nb;
static void reset(void){memset(conf,0,sizeof(conf));memset(inS,0,sizeof(inS));memset(tabu,0,sizeof(tabu));sz=0;for(int i=0;i<nb;i++)addp(base[i]);}
static void recreate(void){
  for(int pass=0;pass<3;pass++){
    int start=rndint(N), step; do{step=1+rndint(N-1);}while(step%3==0);
    uint64_t pos=start;
    for(int k=0;k<N;k++){int p=(int)(pos%N); if(!inS[p]&&!tabu[p]&&conf[p]==0)addp(p); pos+=step;}
  }
}
int main(int argc,char**argv){
  init(); FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1)base[nb++]=v; fclose(f);
  int r=argc>1?atoi(argv[1]):20; int trials=argc>2?atoi(argv[2]):1000;
  int hist[600]; memset(hist,0,sizeof(hist));
  int best=0;
  for(int t=0;t<trials;t++){
    reset();
    for(int i=0;i<r;i++){int p=S[rndint(sz)]; if(inS[p]){remp(p);tabu[p]=1;}}
    recreate();
    if(sz>best)best=sz;
    if(sz>=0&&sz<600)hist[sz]++;
  }
  printf("r=%d best=%d\n",r,best);
  int shown=0;
  for(int i=599;i>=0&&shown<12;i--) if(hist[i]){printf("  %d: %d\n",i,hist[i]);shown++;}
  return 0;
}
