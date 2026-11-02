#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define N 6561
#define MAXS 1200

static uint8_t ADDHI[81][81], ADDLO[81][81], NEGHI[81], NEGLO[81];
static uint16_t S[MAXS];
static uint8_t inS[N];
static int conf[N];
static int sz;

static inline int add4(int a,int b){
    int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;
    int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;
    return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);
}
static inline int neg4(int a){
    int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;
    return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);
}
static inline int third(int p,int q){
    return NEGHI[ADDHI[p/81][q/81]]*81 + NEGLO[ADDLO[p%81][q%81]];
}
static void init_tables(void){
    for(int a=0;a<81;a++){ NEGHI[a]=neg4(a); NEGLO[a]=neg4(a);
        for(int b=0;b<81;b++){ ADDHI[a][b]=add4(a,b); ADDLO[a][b]=add4(a,b); } }
}
static inline void addp(int p){
    inS[p]=1; S[sz++]=p;
    for(int i=0;i<sz-1;i++) conf[third(p,S[i])]++;
}
static inline void remp(int p){
    int pos=-1;
    for(int i=0;i<sz;i++) if(S[i]==p){pos=i;break;}
    if(pos<0){fprintf(stderr,"remp missing %d\n",p);exit(1);}
    S[pos]=S[--sz]; inS[p]=0;
    for(int i=0;i<sz;i++) conf[third(p,S[i])]--;
}

static uint64_t rng_state=0x123456789abcdefULL;
static inline uint64_t rnd(void){ uint64_t x=rng_state; x^=x<<13; x^=x>>7; x^=x<<17; rng_state=x; return x; }
static inline int rndint(int n){ return (int)(rnd()% (uint64_t)n); }

static uint16_t removed[MAXS], added[MAXS];
static int nrem, nadd;

static int best_sz=0;
static uint16_t bestS[MAXS];

/* rebuild conf from scratch */
static void rebuild(void){
    memset(conf,0,sizeof(conf));
    for(int i=0;i<sz;i++) for(int j=i+1;j<sz;j++) conf[third(S[i],S[j])]++;
}

static void save_best(void){
    best_sz=sz; memcpy(bestS,S,sz*sizeof(uint16_t));
}

/* recreate: sweep points in a pseudo-random cyclic order, add if addable */
static void recreate(int passes, uint64_t start, uint64_t step){
    for(int pass=0; pass<passes; pass++){
        uint64_t pos=start;
        for(int k=0;k<N;k++){
            int p=(int)(pos % N);
            if(!inS[p] && conf[p]==0){ addp(p); added[nadd++]=(uint16_t)p; }
            pos += step;
        }
        start += 1237; /* vary */
    }
}

/* find points x outside with small conf, build pair list, greedy cover */
static int pairs_a[64],pairs_b[64],npairs;
static void conflict_pairs(int x){
    npairs=0;
    for(int i=0;i<sz;i++){
        int a=S[i]; int b=third(x,a);
        if(inS[b] && a<b){
            if(npairs<64){pairs_a[npairs]=a;pairs_b[npairs]=b;}
            npairs++;
        }
    }
}

static int cover_pick[64],ncover;
static void greedy_cover(void){
    int covered[64]; memset(covered,0,sizeof(int)*npairs);
    ncover=0;
    while(1){
        int bestv=-1,bestc=0;
        for(int i=0;i<sz;i++){
            int v=S[i],c=0;
            for(int j=0;j<npairs;j++) if(!covered[j] && (pairs_a[j]==v||pairs_b[j]==v)) c++;
            if(c>bestc){bestc=c;bestv=v;}
        }
        if(bestv<0) break;
        for(int j=0;j<npairs;j++) if(pairs_a[j]==bestv||pairs_b[j]==bestv) covered[j]=1;
        cover_pick[ncover++]=bestv;
        if(ncover>=60) break;
    }
}

static void undo_move(void){
    for(int i=nadd-1;i>=0;i--) remp(added[i]);
    for(int i=nrem-1;i>=0;i--) addp(removed[i]);
    nadd=nrem=0;
}

static void report(long iters, int cur){
    printf("iters=%ld cur=%d best=%d\n", iters, cur, best_sz); fflush(stdout);
}

int main(int argc,char**argv){
    init_tables();
    FILE*f=fopen("baseline.idx","r");
    if(!f){perror("baseline.idx");return 1;}
    int v; while(fscanf(f,"%d",&v)==1) addp(v);
    fclose(f);
    save_best();
    printf("start size %d\n",sz); fflush(stdout);

    long iters = argc>1? atol(argv[1]) : 2000000;
    long t0=time(NULL);
    for(long it=1; it<=iters; it++){
        nadd=nrem=0;
        int mode = rndint(100);

        if(mode < 55){
            /* targeted insertion of a low-conflict outside point */
            int x=-1,bc=1000;
            for(int t=0;t<48;t++){
                int c=rndint(N);
                if(inS[c]) continue;
                if(conf[c]<bc){bc=conf[c];x=c;}
            }
            if(x>=0){
                conflict_pairs(x);
                if(npairs>0 && npairs<=64){
                    greedy_cover();
                    /* remove cover vertices (limit 5) */
                    int lim = ncover<5?ncover:5;
                    for(int i=0;i<lim;i++){
                        int p=cover_pick[i];
                        if(inS[p]){ remp(p); removed[nrem++]=(uint16_t)p; }
                    }
                    if(conf[x]==0){ addp(x); added[nadd++]=(uint16_t)x; }
                }
            }
        } else {
            /* random ruin */
            int r=4+rndint(14);
            for(int i=0;i<r && sz>0;i++){
                int p=S[rndint(sz)];
                if(inS[p]){ remp(p); removed[nrem++]=(uint16_t)p; }
            }
        }

        if(nadd+nrem==0) continue;
        recreate(2, rnd(), 2+(rnd()%(N-1)));

        if(sz>best_sz){
            save_best();
            printf("NEW BEST %d at iter %ld (time %lds)\n",sz,it,(long)(time(NULL)-t0)); fflush(stdout);
            if(sz>=513){
                FILE*o=fopen("best.idx","w");
                for(int i=0;i<best_sz;i++) fprintf(o,"%d\n",bestS[i]);
                fclose(o);
                printf("FOUND %d\n",best_sz); return 0;
            }
        } else if(sz<best_sz){
            undo_move();
        }
        if(it%200000==0) report(it,sz);
    }
    FILE*o=fopen("best.idx","w");
    for(int i=0;i<best_sz;i++) fprintf(o,"%d\n",bestS[i]);
    fclose(o);
    printf("done best %d\n",best_sz);
    return 0;
}
