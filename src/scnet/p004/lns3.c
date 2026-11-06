#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#define NP 16807
#define NS 16807
#define W 263
static uint64_t *Nmask;            // shared
static uint64_t *baseB;            // shared baseline blocking
static uint64_t fullmask[W];
static int base_gc[NP];
static uint64_t *baseT[NP], *baseU[NP];
static int gsizes[NP];
static int nonempty[NP], nne=0;

static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static inline int enc7(const int d[5]){int r=0;for(int i=0;i<5;i++)r=r*7+d[i];return r;}
static void build_neighbors(void){
 int d[5];
 for(int s=0;s<NS;s++){digits7(s,d);uint64_t*m=Nmask+(size_t)s*W;int a[5][3];
  for(int k=0;k<5;k++){a[k][0]=(d[k]+6)%7;a[k][1]=d[k];a[k][2]=(d[k]+1)%7;}
  for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
   int e[5]={a[0][i0],a[1][i1],a[2][i2],a[3][i3],a[4][i4]};int t=enc7(e);m[t>>6]|=1ULL<<(t&63);}}
}

static void wr7(FILE*o,int v){char b[6];for(int i=4;i>=0;i--){b[i]='0'+(v%7);v/=7;}b[5]=0;fputs(b,o);}
static inline uint64_t rng(uint64_t *s){uint64_t x=*s;x^=x<<13;x^=x>>7;x^=x<<17;*s=x;return x;}

typedef struct {
 uint64_t *B; uint64_t **T; uint64_t **U; int *gc; int total; uint64_t seed;
} St;

static void st_init(St*st,uint64_t seed){
 st->B=malloc((size_t)NP*W*8); memcpy(st->B,baseB,(size_t)NP*W*8);
 st->T=calloc(NP,sizeof(uint64_t*)); st->U=calloc(NP,sizeof(uint64_t*));
 st->gc=calloc(NP,sizeof(int)); st->seed=seed; st->total=0;
 for(int i=0;i<nne;i++){int q=nonempty[i]; st->T[q]=malloc(W*8); st->U[q]=malloc(W*8);
  memcpy(st->T[q],baseT[q],W*8); memcpy(st->U[q],baseU[q],W*8); st->gc[q]=base_gc[q]; st->total+=base_gc[q];}
}

// extract all points into buf (prefix*16807+suffix), returns count
static int st_dump(St*st,int *buf){
 int n=0;
 for(int q=0;q<NP;q++){ if(!st->gc[q])continue; uint64_t*T=st->T[q];
  for(int w=0;w<W;w++){uint64_t x=T[w];while(x){int b=__builtin_ctzll(x);x&=x-1;buf[n++]=q*NS+w*64+b;}}}
 return n;
}

int main(int argc,char**argv){
 long iters = argc>1?atol(argv[1]):1000000;
 int nth = argc>2?atoi(argv[2]):omp_get_max_threads();
 const char*outf = argc>3?argv[3]:"/tmp/lns_best.txt";
 Nmask=calloc((size_t)NS*W,8); baseB=calloc((size_t)NP*W,8);
 for(int w=0;w<W;w++)fullmask[w]=~0ULL; fullmask[W-1]=(1ULL<<(NS-(W-1)*64))-1;
 build_neighbors();
 FILE*f=fopen("/tmp/p004_points.txt","r"); if(!f){perror("points");return 1;}
 char buf[64]; static int pref[200000],suf[200000]; int n=0;
 while(fscanf(f,"%63s",buf)==1){int d[10];for(int i=0;i<10;i++)d[i]=buf[i]-'0';int p=0,s=0;
  for(int i=0;i<5;i++)p=p*7+d[i];for(int i=5;i<10;i++)s=s*7+d[i];pref[n]=p;suf[n]=s;base_gc[p]++;n++;}
 fclose(f);
 for(int i=0;i<n;i++){int p=pref[i],s=suf[i];
  if(!baseT[p])baseT[p]=calloc(W,8);
  baseT[p][s>>6]|=1ULL<<(s&63);}
 for(int p=0;p<NP;p++){ if(!base_gc[p])continue; nonempty[nne++]=p; gsizes[p]=base_gc[p];
  baseU[p]=calloc(W,8); uint64_t*u=baseU[p];
  for(int w=0;w<W;w++){uint64_t x=baseT[p][w];while(x){int b=__builtin_ctzll(x);x&=x-1;int s=w*64+b;const uint64_t*m=Nmask+(size_t)s*W;for(int j=0;j<W;j++)u[j]|=m[j];}}
 }
 // baseline B: for each q, OR U over neighbors
 #pragma omp parallel for schedule(dynamic)
 for(int q=0;q<NP;q++){
  uint64_t*Bq=baseB+(size_t)q*W; const uint64_t*mq=Nmask+(size_t)q*W;
  for(int w=0;w<W;w++){uint64_t x=mq[w];while(x){int b=__builtin_ctzll(x);x&=x-1;int r=w*64+b;
   if(base_gc[r]){const uint64_t*u=baseU[r];for(int j=0;j<W;j++)Bq[j]|=u[j];}}}
 }
 fprintf(stderr,"loaded n=%d groups=%d\n",n,nne);
 static volatile long best=0; static int *bestbuf; bestbuf=malloc(sizeof(int)*300000);
 #pragma omp parallel num_threads(nth)
 {
  St st; st_init(&st,0x9E3779B97F4A7C15ULL ^ (uint64_t)(omp_get_thread_num()+1)*0x1234567ULL);
  if(omp_get_thread_num()==0) fprintf(stderr,"init total %d\n",st.total);
  uint64_t *saveB[2]; int savp[2],savq[2]; long localbest=st.total;
  // per-iteration scratch for saving B of N[W]
  static __thread uint64_t *bufB=NULL; if(!bufB)bufB=malloc((size_t)500*W*8);
  static __thread int *tbuf=NULL; if(!tbuf)tbuf=malloc(sizeof(int)*300000);
  int *nbrs=malloc(sizeof(int)*300);
  for(long it=0; it<iters; it++){
   // pick p: 70% small group (<=26), 30% random nonempty
   int p;
   if((rng(&st.seed)%100)<70){
    int tries=0; do{p=nonempty[rng(&st.seed)%nne];tries++;}while(gsizes[p]>26&&tries<200);
   } else p=nonempty[rng(&st.seed)%nne];
   if(!st.gc[p])continue;
   // pick q: random neighbor of p with gc>0 (must be adjacent prefix)
   const uint64_t*mp=Nmask+(size_t)p*W; int nc=0;
   for(int w=0;w<W;w++){uint64_t x=mp[w];while(x){int b=__builtin_ctzll(x);x&=x-1;int r=w*64+b;if(r!=p&&st.gc[r]>0)nbrs[nc++]=r;}}
   if(nc==0)continue;
   int q=nbrs[rng(&st.seed)%nc];
   if(q==p)continue;
   int oldtot=st.total;
   // save T/U/gc for p,q and B for N[p] union N[q]
   int Wl[2]={p,q};
   uint64_t *sT[2],*sU[2]; int sG[2];
   for(int k=0;k<2;k++){int w=Wl[k]; sG[k]=st.gc[w]; sT[k]=st.T[w]; sU[k]=st.U[w];}
   // collect affected prefixes
   static __thread int aff[600]; int na=0;
   for(int k=0;k<2;k++){int w=Wl[k];const uint64_t*mw=Nmask+(size_t)w*W;
    for(int ww=0;ww<W;ww++){uint64_t x=mw[ww];while(x){int b=__builtin_ctzll(x);x&=x-1;int r=ww*64+b;aff[na++]=r;}}}
   // dedup aff (simple O(na^2) since na<=486)
   int na2=0; for(int i=0;i<na;i++){int r=aff[i],dup=0;for(int j=0;j<na2;j++)if(aff[j]==r){dup=1;break;}if(!dup)aff[na2++]=r;}
   // save B
   for(int i=0;i<na2;i++) memcpy(bufB+(size_t)i*W, st.B+(size_t)aff[i]*W, W*8);
   // remove p,q
   for(int k=0;k<2;k++){int w=Wl[k]; if(st.T[w]){memset(st.T[w],0,W*8); memset(st.U[w],0,W*8);} st.gc[w]=0;}
   st.total -= (sG[0]+sG[1]);
   // recompute B for aff
   for(int i=0;i<na2;i++){int r=aff[i];uint64_t*Br=st.B+(size_t)r*W;memset(Br,0,W*8);
    const uint64_t*mr=Nmask+(size_t)r*W;
    for(int ww=0;ww<W;ww++){uint64_t x=mr[ww];while(x){int b=__builtin_ctzll(x);x&=x-1;int r2=ww*64+b;
     if(st.gc[r2]){const uint64_t*u=st.U[r2];for(int j=0;j<W;j++)Br[j]|=u[j];}}}
   }
   // refill: iterate aff in random order repeatedly, add allowed
   int added=0;
   for(int pass=0;pass<3;pass++){
    int progress=0;
    for(int ii=0;ii<na2;ii++){
     int r=aff[ii];
     if(!st.T[r]){st.T[r]=calloc(W,8);st.U[r]=calloc(W,8);}
     uint64_t*Br=st.B+(size_t)r*W;
     // candidates: ~B & ~T
     for(int ww=0;ww<W;ww++){
      uint64_t x=(~Br[ww])&fullmask[ww]&~st.T[r][ww]&~(st.U[r][ww]);
      while(x){int b=__builtin_ctzll(x);x&=x-1;int s=ww*64+b;
       // add s at r
       st.T[r][s>>6]|=1ULL<<(s&63); st.gc[r]++; st.total++; added++;
       const uint64_t*ms=Nmask+(size_t)s*W;
       {uint64_t*ur=st.U[r]; for(int j=0;j<W;j++)ur[j]|=ms[j];}
       // update B for neighbors of r
       const uint64_t*mr2=Nmask+(size_t)r*W;
       for(int w2=0;w2<W;w2++){uint64_t y=mr2[w2];while(y){int b2=__builtin_ctzll(y);y&=y-1;int r3=w2*64+b2;
        uint64_t*B3=st.B+(size_t)r3*W; for(int j=0;j<W;j++)B3[j]|=ms[j];}}
       progress=1; break;
      }
     }
    }
    if(!progress)break;
   }
   if(st.total>localbest){localbest=st.total;}
   if(st.total>best){
    #pragma omp critical
    { if(st.total>best){ best=st.total; int cnt=st_dump(&st,bestbuf); FILE*o=fopen(outf,"w"); for(int i=0;i<cnt;i++){wr7(o,bestbuf[i]/NS);wr7(o,bestbuf[i]%NS);fputc(10,o);} fclose(o);
      fprintf(stderr,"NEW BEST %ld\n",(long)best); } }
   }
   if((it&0x3FFF)==0){ /* occasional reset from baseline if drifted low */
    if(st.total < n-40){
     // reset state from baseline: clear all groups first
     for(int r=0;r<NP;r++){ if(st.T[r]){memset(st.T[r],0,W*8);memset(st.U[r],0,W*8);} st.gc[r]=0; }
     for(int i=0;i<nne;i++){int r=nonempty[i];memcpy(st.T[r],baseT[r],W*8);memcpy(st.U[r],baseU[r],W*8);st.gc[r]=base_gc[r];}
     memcpy(st.B,baseB,(size_t)NP*W*8); st.total=n;
    }
   }
  }
  free(bufB); free(tbuf); free(nbrs);
 }
 printf("best %ld\n",best);
 return 0;
}
