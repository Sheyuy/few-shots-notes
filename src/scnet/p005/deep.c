#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#define N 6561
#define MAXS 1400
static uint8_t ADDHI[81][81],ADDLO[81][81],NEGHI[81],NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N],tabu[N]; static int conf[N]; static int sz;
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static void remp(int p){int pos=-1;for(int i=0;i<sz;i++)if(S[i]==p){pos=i;break;}S[pos]=S[--sz];inS[p]=0;for(int i=0;i<sz;i++)conf[third(p,S[i])]--;}
static uint64_t rng=0x9E3779B97F4A7C15ULL;
static inline uint64_t rnd(void){uint64_t x=rng;x^=x<<13;x^=x>>7;x^=x<<17;rng=x;return x;}
static inline int rndint(int n){return (int)(rnd()%(uint64_t)n);}
static uint16_t base[MAXS]; static int nb;
static int best=0; static uint16_t bestS[MAXS];
static void reset(void){memset(conf,0,sizeof(conf));memset(inS,0,sizeof(inS));memset(tabu,0,sizeof(tabu));sz=0;for(int i=0;i<nb;i++)addp(base[i]);}
static void refill(void){for(int pass=0;pass<4;pass++){int st;do{st=1+rndint(N-1);}while(st%3==0);uint64_t pos=rnd()%N;for(int k=0;k<N;k++){int p=(int)(pos%N);if(!inS[p]&&!tabu[p]&&conf[p]==0)addp(p);pos+=st;}}}
int main(int argc,char**argv){
    init(); FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1)base[nb++]=v; fclose(f);
    int trials=argc>1?atoi(argv[1]):200;
    long t0=time(NULL);
    for(int x=0;x<N;x++){
        reset(); if(inS[x]||conf[x]!=4) continue;
        /* collect pairs */
        int pa[64],pb[64],np=0;
        for(int i=0;i<sz;i++){int a=S[i],b=third(x,a);if(inS[b]&&a<b&&np<64){pa[np]=a;pb[np]=b;np++;}}
        if(np!=4) continue;
        int bestlocal=0;
        for(int t=0;t<trials;t++){
            reset();
            for(int i=0;i<np;i++){remp(pa[i]);tabu[pa[i]]=1;remp(pb[i]);tabu[pb[i]]=1;}
            refill();
            if(sz>bestlocal)bestlocal=sz;
            if(sz>best){best=sz;memcpy(bestS,S,sz*sizeof(uint16_t));printf("global best %d (x=%d t=%d)\n",best,x,t);fflush(stdout);
                FILE*o=fopen("deep_best.idx","w");for(int i=0;i<sz;i++)fprintf(o,"%d\n",bestS[i]);fclose(o);
                if(best>=513){printf("FOUND %d\n",best);return 0;}}
        }
        printf("x=%d local best %d (elapsed %lds)\n",x,bestlocal,(long)(time(NULL)-t0));fflush(stdout);
    }
    printf("done best %d\n",best);
    return 0;
}
