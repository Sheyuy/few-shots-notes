#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define N 6561
#define K 513
#define MAXS 700
static uint8_t ADDHI[81][81], ADDLO[81][81], NEGHI[81], NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N]; static int conf[N]; static int sz;
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static void remp(int p){int pos=-1;for(int i=0;i<sz;i++)if(S[i]==p){pos=i;break;}S[pos]=S[--sz];inS[p]=0;for(int i=0;i<sz;i++)conf[third(p,S[i])]--;}
static int energy(void){int e=0;for(int i=0;i<sz;i++)e+=conf[S[i]];return e/3;}

static uint64_t rng=0x243f6a8885a308d3ULL;
static inline uint64_t rnd(void){uint64_t x=rng;x^=x<<13;x^=x>>7;x^=x<<17;rng=x;return x;}
static inline int rndint(int n){return (int)(rnd()%(uint64_t)n);}

static uint16_t base[MAXS]; static int nb;

int main(int argc,char**argv){
    init();
    FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1){base[nb++]=v;}
    fclose(f);
    long iters = argc>1?atol(argv[1]):50000000;
    long deadline = argc>2?atol(argv[2]):120;
    long t0=time(NULL);
    int bestE=1000000; long bestit=0;
    int restart=0;
    restart:
    restart++;
    memset(conf,0,sizeof(conf)); memset(inS,0,sizeof(inS)); sz=0;
    for(int i=0;i<nb;i++) addp(base[i]);
    /* add one random low-conf outside point */
    int bx=-1,bc=100000;
    for(int t=0;t<2000;t++){int p=rndint(N); if(!inS[p]&&conf[p]<bc){bc=conf[p];bx=p;}}
    if(bx<0){printf("no outside?\n");return 1;}
    addp(bx);
    int E=energy();

    for(long it=0; it<iters; it++){
        if((it&0x3fff)==0 && time(NULL)-t0>deadline){ printf("time up bestE=%d\n",bestE); return 0; }
        if(E==0){
            printf("FOUND 0 energy at restart %d it %ld\n",restart,it);
            FILE*o=fopen("found.idx","w"); for(int i=0;i<sz;i++) fprintf(o,"%d\n",S[i]); fclose(o);
            return 0;
        }
        /* pick a violated vertex */
        int p=-1;
        for(int t=0;t<200;t++){int q=S[rndint(sz)]; if(conf[q]>0){p=q;break;}}
        if(p<0){ /* find any */ for(int i=0;i<sz;i++) if(conf[S[i]]>0){p=S[i];break;} }
        if(p<0) break;
        /* find one conflicting pair (a,b): a+b=-p */
        int a=-1,b=-1;
        for(int i=0;i<sz;i++){int q=S[i]; if(q==p) continue; int r=third(p,q); if(inS[r]&&r!=q){a=q;b=r;break;}}
        if(a<0){continue;}
        /* choose vertex to remove: min conf among p,a,b with random tie */
        int cand[3]={p,a,b}; int mi=0,mc=conf[cand[0]];
        for(int i=1;i<3;i++) if(conf[cand[i]]<mc){mc=conf[cand[i]];mi=i;}
        int rem=cand[mi];
        remp(rem);
        /* choose point to add: best of sample by conf */
        int add=-1,ac=1000000;
        for(int t=0;t<128;t++){int q=rndint(N); if(inS[q]||q==rem) continue; if(conf[q]<ac){ac=conf[q];add=q;}}
        if(add<0){addp(rem);continue;}
        addp(add);
        E=energy();
        if(E<bestE){bestE=E;bestit=it; printf("restart %d it %ld E=%d\n",restart,it,E); fflush(stdout);}
        /* restart if no improvement for long */
        if(it-bestit>2000000){ printf("restart %d ended E=%d\n",restart,E); fflush(stdout); goto restart; }
    }
    printf("done bestE=%d restart=%d\n",bestE,restart);
    return 0;
}
