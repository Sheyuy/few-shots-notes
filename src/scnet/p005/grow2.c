#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#define N 6561
#define MAXS 1400
static uint8_t ADDHI[81][81],ADDLO[81][81],NEGHI[81],NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N],tabu[N]; static int conf[N]; static int sz;
static uint16_t removed[MAXS],added[MAXS]; static int nrem,nadd;
static int best=0; static uint16_t bestS[MAXS];

static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static void remp(int p){int pos=-1;for(int i=0;i<sz;i++)if(S[i]==p){pos=i;break;}if(pos<0){printf("remp missing\n");exit(3);}S[pos]=S[--sz];inS[p]=0;for(int i=0;i<sz;i++)conf[third(p,S[i])]--;}
static uint64_t rng=0x2545F4914F6CDD1DULL;
static inline uint64_t rnd(void){uint64_t x=rng;x^=x<<13;x^=x>>7;x^=x<<17;rng=x;return x;}
static inline int rndint(int n){return (int)(rnd()%(uint64_t)n);}
static uint64_t step(void){uint64_t s; do{s=1+rnd()%(N-1);}while(s%3==0); return s;}
static void refill(void){for(int pass=0;pass<2;pass++){uint64_t pos=rnd()%N;uint64_t st=step();for(int k=0;k<N;k++){int p=(int)(pos%N);if(!inS[p]&&!tabu[p]&&conf[p]==0){addp(p);added[nadd++]=(uint16_t)p;}pos+=st;}}}
static int recompute_ok(void){
    static int c2[N]; memset(c2,0,sizeof(c2));
    for(int i=0;i<sz;i++)for(int j=i+1;j<sz;j++)c2[third(S[i],S[j])]++;
    for(int i=0;i<sz;i++) if(c2[S[i]]!=0) return 0;
    return 1;
}
static void save(const char*name){FILE*o=fopen(name,"w");for(int i=0;i<sz;i++)fprintf(o,"%d\n",S[i]);fclose(o);}
int main(int argc,char**argv){
    init(); long secs=argc>1?atol(argv[1]):300; if(argc>2) rng^=strtoull(argv[2],0,10)*1234567;
    long t0=time(NULL);
    int perm[N]; for(int i=0;i<N;i++)perm[i]=i;
    for(int i=N-1;i>0;i--){int j=rndint(i+1);int t=perm[i];perm[i]=perm[j];perm[j]=t;}
    for(int i=0;i<N;i++){int p=perm[i];if(conf[p]==0)addp(p);}
    printf("greedy start %d\n",sz); fflush(stdout);
    if(!recompute_ok()){printf("corrupt start\n");return 2;}
    best=sz; memcpy(bestS,S,sz*sizeof(uint16_t));
    long it=0;
    while(time(NULL)-t0<secs){
        it++; nrem=nadd=0;
        int r=4+rndint(12);
        for(int i=0;i<r&&sz>0;i++){int p=S[rndint(sz)];if(inS[p]){remp(p);removed[nrem++]=p;tabu[p]=1;}}
        refill();
        if(sz>best){
            if(!recompute_ok()){printf("CORRUPT it=%ld sz=%d\n",it,sz);return 2;}
            best=sz; memcpy(bestS,S,sz*sizeof(uint16_t));
            printf("it=%ld NEW BEST %d (t=%ld)\n",it,best,(long)(time(NULL)-t0));fflush(stdout);
            FILE*o=fopen("grow2_best.idx","w");for(int i=0;i<best;i++)fprintf(o,"%d\n",bestS[i]);fclose(o);
            if(best>=513){printf("FOUND %d\n",best);return 0;}
        } else if(sz<best){
            for(int i=nadd-1;i>=0;i--){int p=added[i];if(inS[p])remp(p);}
            for(int i=nrem-1;i>=0;i--){int p=removed[i];if(!inS[p])addp(p);}
        }
        for(int i=0;i<nrem;i++)tabu[removed[i]]=0;
        if((it&65535)==0){printf("it=%ld sz=%d best=%d t=%ld\n",it,sz,best,(long)(time(NULL)-t0));fflush(stdout);}
    }
    save("grow2_final.idx");
    printf("done best=%d it=%ld\n",best,it);
    return 0;
}
