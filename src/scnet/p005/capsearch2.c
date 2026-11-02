#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define N 6561
#define MAXS 1200

static uint8_t ADDHI[81][81], ADDLO[81][81], NEGHI[81], NEGLO[81];
static uint16_t S[MAXS];
static uint8_t inS[N], tabu[N];
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

static uint64_t rng_state=0x9e3779b97f4a7c15ULL;
static inline uint64_t rnd(void){ uint64_t x=rng_state; x^=x<<13; x^=x>>7; x^=x<<17; rng_state=x; return x; }
static inline int rndint(int n){ return (int)(rnd()% (uint64_t)n); }

static uint16_t removed[MAXS], added[MAXS];
static int nrem, nadd;
static int best_sz=0;
static uint16_t bestS[MAXS];

static void save_best(void){ best_sz=sz; memcpy(bestS,S,sz*sizeof(uint16_t)); }

static uint64_t coprime_step(void){
    uint64_t s;
    do { s = 1 + rnd()%(N-1); } while(s%3==0);
    return s;
}
static void recreate(int passes, uint64_t start, uint64_t step){
    for(int pass=0; pass<passes; pass++){
        uint64_t pos=start;
        for(int k=0;k<N;k++){
            int p=(int)(pos % N);
            if(!inS[p] && !tabu[p] && conf[p]==0){ addp(p); added[nadd++]=(uint16_t)p; }
            pos += step;
        }
        start += 1237;
    }
}

static int pairs_a[160],pairs_b[160],npairs;
static void conflict_pairs(int x){
    npairs=0;
    for(int i=0;i<sz;i++){
        int a=S[i]; int b=third(x,a);
        if(inS[b] && a<b){
            if(npairs<160){pairs_a[npairs]=a;pairs_b[npairs]=b;}
            npairs++;
        }
    }
}
static int cover_pick[160],ncover;
static void greedy_cover(void){
    static int covered[160]; memset(covered,0,sizeof(int)*npairs);
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
    }
}
static void undo_move(void){
    for(int i=nadd-1;i>=0;i--) remp(added[i]);
    for(int i=nrem-1;i>=0;i--) addp(removed[i]);
    nadd=nrem=0;
}
static void clear_tabu(void){ for(int i=0;i<nrem;i++) tabu[removed[i]]=0; }

static void write_idx(const char*name){
    FILE*o=fopen(name,"w");
    for(int i=0;i<best_sz;i++) fprintf(o,"%d\n",bestS[i]);
    fclose(o);
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
    long hist[40]; memset(hist,0,sizeof(hist));
    long equals=0, worse=0;

    for(long it=1; it<=iters; it++){
        nadd=nrem=0;
        int mode = rndint(100);
        int addedx=0;

        if(mode < 65){
            int x=-1,bc=1000;
            for(int t=0;t<64;t++){
                int c=rndint(N);
                if(inS[c]) continue;
                if(conf[c]<bc){bc=conf[c];x=c;}
            }
            if(x>=0){
                conflict_pairs(x);
                if(npairs==0){
                    addp(x); save_best(); write_idx("best.idx");
                    printf("FOUND addable %d size %d at iter %ld\n",x,sz,it); return 0;
                }
                if(npairs<=150){
                    greedy_cover();
                    if(ncover<=4){
                        for(int i=0;i<ncover;i++){
                            int p=cover_pick[i];
                            if(inS[p]){ remp(p); removed[nrem++]=(uint16_t)p; tabu[p]=1; }
                        }
                        if(conf[x]==0){ addp(x); added[nadd++]=(uint16_t)x; addedx=1; }
                    }
                }
            }
        } else {
            int r=3+rndint(9);
            for(int i=0;i<r && sz>0;i++){
                int p=S[rndint(sz)];
                if(inS[p]){ remp(p); removed[nrem++]=(uint16_t)p; tabu[p]=1; }
            }
        }

        if(nadd+nrem==0){ clear_tabu(); continue; }
        recreate(2, rnd()%N, coprime_step());

        if(sz>best_sz){
            save_best(); write_idx("best.idx");
            printf("NEW BEST %d at iter %ld (t=%lds)\n",sz,it,(long)(time(NULL)-t0)); fflush(stdout);
        } else if(sz<best_sz){
            undo_move(); worse++;
        } else { equals++; }
        if(sz-480>=0 && sz-480<40) hist[sz-480]++;
        clear_tabu();
        if(it%500000==0){
            printf("it=%ld cur=%d best=%d eq=%ld worse=%ld\n",it,sz,best_sz,equals,worse); fflush(stdout);
        }
    }
    write_idx("best.idx");
    printf("done best %d\n",best_sz);
    for(int i=0;i<40;i++) if(hist[i]) printf("size %d: %ld\n",i+480,hist[i]);
    return 0;
}
