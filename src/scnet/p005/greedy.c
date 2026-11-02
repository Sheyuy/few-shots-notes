#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define N 6561
#define MAXS 900
static uint8_t ADDHI[81][81],ADDLO[81][81],NEGHI[81],NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N]; static int conf[N]; static int sz;
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static uint64_t rng=0xdeadbeefcafebabeULL;
static inline uint64_t rnd(void){uint64_t x=rng;x^=x<<13;x^=x>>7;x^=x<<17;rng=x;return x;}
static inline int rndint(int n){return (int)(rnd()%(uint64_t)n);}
int main(int argc,char**argv){
    init(); int trials=argc>1?atoi(argv[1]):100; int best=0; long hist[900]; memset(hist,0,sizeof(hist));
    int perm[N];
    for(int t=0;t<trials;t++){
        memset(conf,0,sizeof(conf)); memset(inS,0,sizeof(inS)); sz=0;
        for(int i=0;i<N;i++)perm[i]=i;
        for(int i=N-1;i>0;i--){int j=rndint(i+1);int tmp=perm[i];perm[i]=perm[j];perm[j]=tmp;}
        for(int i=0;i<N;i++){int p=perm[i]; if(conf[p]==0) addp(p);}
        hist[sz]++; if(sz>best)best=sz;
    }
    printf("greedy best=%d\n",best);
    int shown=0; for(int i=899;i>=0&&shown<10;i--) if(hist[i]){printf("  %d: %ld\n",i,hist[i]);shown++;}
    return 0;
}
