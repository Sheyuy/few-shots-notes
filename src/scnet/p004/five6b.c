#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#define NP 16807
#define NS 16807
#define W 263
#define N 282475249ULL
#define MASK29 0x1FFFFFFFULL
typedef unsigned __int128 u128;

static uint64_t *Nmask,*T; static int *gcnt;
static uint16_t *mm;

static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static inline int enc7(const int d[5]){int r=0;for(int i=0;i<5;i++)r=r*7+d[i];return r;}
static void build_neighbors(void){
 int d[5];
 for(int s=0;s<NS;s++){
  digits7(s,d);uint64_t*m=Nmask+(size_t)s*W;int a[5][3];
  for(int k=0;k<5;k++){a[k][0]=(d[k]+6)%7;a[k][1]=d[k];a[k][2]=(d[k]+1)%7;}
  for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
   int e[5]={a[0][i0],a[1][i1],a[2][i2],a[3][i3],a[4][i4]};
   int t=enc7(e);m[t>>6]|=1ULL<<(t&63);
  }
 }
}
static inline int adj10(uint32_t x,uint32_t y){
 int qx=x/NS,sx=x%NS,qy=y/NS,sy=y%NS;
 int dx[5],dy[5],ex[5],ey[5];
 digits7(qx,dx);digits7(qy,dy);digits7(sx,ex);digits7(sy,ey);
 for(int i=0;i<5;i++){int e=(dx[i]-dy[i])%7;if(e>1&&e<6)return 0;}
 for(int i=0;i<5;i++){int e=(ex[i]-ey[i])%7;if(e>1&&e<6)return 0;}
 return 1;
}
typedef struct { u128 key; uint32_t pts[8]; uint8_t n; } Ent;
typedef struct { Ent *e; uint64_t mask; } Tab;
static inline uint64_t h64(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;}
static inline uint64_t h128(u128 x){uint64_t lo=(uint64_t)x,hi=(uint64_t)(x>>64);return h64(lo^(hi*0x9E3779B97F4A7C15ULL));}
static void tab_init(Tab*t,uint64_t sz){t->e=calloc(sz,sizeof(Ent));if(!t->e){fprintf(stderr,"oom\n");exit(1);}t->mask=sz-1;}
static Ent* tab_find(Tab*t,u128 key){uint64_t h=h128(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}return NULL;}
static Ent* tab_put(Tab*t,u128 key){uint64_t h=h128(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}t->e[h].key=key+1;return &t->e[h];}

typedef struct { u128 key; uint32_t w; } Rec128;
static int cmp128(const void*a,const void*b){const Rec128*x=a,*y=b;if(x->key<y->key)return -1;if(x->key>y->key)return 1;return 0;}
typedef struct { uint64_t key; uint32_t w; } Rec64;
static int cmp64(const void*a,const void*b){const Rec64*x=a,*y=b;if(x->key<y->key)return -1;if(x->key>y->key)return 1;return 0;}
static long lb128(Rec128*a,long n,u128 key){long lo=0,hi=n;while(lo<hi){long m=(lo+hi)/2;if(a[m].key<key)lo=m+1;else hi=m;}return lo;}
static long lb64(Rec64*a,long n,uint64_t key){long lo=0,hi=n;while(lo<hi){long m=(lo+hi)/2;if(a[m].key<key)lo=m+1;else hi=m;}return lo;}

static int hasrec(uint64_t avail,int need,uint64_t *am){
 if(need==0)return 1;
 if(__builtin_popcountll(avail)<need)return 0;
 int v=__builtin_ctzll(avail);
 uint64_t rest=avail&~(1ULL<<v);
 uint64_t av2=rest&~am[v]&~((v==63)?0ULL:((1ULL<<(v+1))-1));
 if(hasrec(av2,need-1,am))return 1;
 return hasrec(rest,need,am);
}
static inline void sort4(uint32_t*v){
 for(int i=0;i<4;i++)for(int j=i+1;j<4;j++)if(v[j]<v[i]){uint32_t t=v[i];v[i]=v[j];v[j]=t;}
}
static inline void sort3(uint32_t*v){
 if(v[1]<v[0]){uint32_t t=v[0];v[0]=v[1];v[1]=t;}
 if(v[2]<v[1]){uint32_t t=v[1];v[1]=v[2];v[2]=t;}
 if(v[1]<v[0]){uint32_t t=v[0];v[0]=v[1];v[1]=t;}
}
int main(void){
 Nmask=calloc((size_t)NS*W,8);
 T=calloc((size_t)NP*W,8);
 gcnt=calloc(NP,sizeof(int));
 build_neighbors();
 FILE*f=fopen("/tmp/p004_points.txt","r");
 if(!f){perror("points");return 1;}
 char buf[32];long np=0;
 while(fscanf(f,"%31s",buf)==1){
  int d[10];for(int i=0;i<10;i++)d[i]=buf[i]-'0';
  int q=0,s=0;for(int i=0;i<5;i++)q=q*7+d[i];for(int i=5;i<10;i++)s=s*7+d[i];
  T[(size_t)q*W+(s>>6)]|=1ULL<<(s&63);gcnt[q]++;np++;
 }
 fclose(f);
 fprintf(stderr,"points %ld\n",np);
 uint16_t*a=malloc(N*2);uint16_t*b=malloc(N*2);
 memset(a,0,N*2);memset(b,0,N*2);
 for(int q=0;q<NP;q++){
  uint64_t*tp=T+(size_t)q*W;
  for(int w=0;w<W;w++){uint64_t x=tp[w];while(x){int bb=__builtin_ctzll(x);x&=x-1;a[(size_t)q*NS+w*64+bb]=1;}}
 }
 for(int dim=0;dim<10;dim++){
  uint64_t sk=1;for(int i=0;i<9-dim;i++)sk*=7;
  uint64_t hi_count=1;for(int i=0;i<dim;i++)hi_count*=7;
  uint64_t lo_count=1;for(int i=0;i<9-dim;i++)lo_count*=7;
  #pragma omp parallel for schedule(static)
  for(uint64_t hi=0;hi<hi_count;hi++){
   uint64_t base0=hi*7*sk;
   for(uint64_t lo=0;lo<lo_count;lo++){
    uint64_t idx0=base0+lo;uint16_t v[7];
    for(int d=0;d<7;d++)v[d]=a[idx0+(uint64_t)d*sk];
    for(int d=0;d<7;d++)b[idx0+(uint64_t)d*sk]=(uint16_t)(v[(d+6)%7]+v[d]+v[(d+1)%7]);
   }
  }
  uint16_t*tmp=a;a=b;b=tmp;
 }
 free(b); mm=a;
 // collect m=4 records
 long cap4=3000000;
 u128 *k4=malloc(sizeof(u128)*cap4); uint32_t *p4=malloc(cap4*4); long n4=0;
 #pragma omp parallel
 {
  u128 *lk=malloc(sizeof(u128)*100000); uint32_t *lp=malloc(100000*4); long ln=0;
  #pragma omp for schedule(dynamic,8)
  for(int q=0;q<NP;q++){
   int nbr[243],nn=0;const uint64_t*mq=Nmask+(size_t)q*W;
   for(int w=0;w<W;w++){uint64_t x=mq[w];while(x){int bb=__builtin_ctzll(x);x&=x-1;int p=w*64+bb;if(gcnt[p])nbr[nn++]=p;}}
   for(int s=0;s<NS;s++){
    uint64_t idx=(uint64_t)q*NS+s;
    if(mm[idx]!=4)continue;
    if((T[(size_t)q*W+(s>>6)]>>(s&63))&1)continue;
    uint64_t bl[4];int nb=0;const uint64_t*nm=Nmask+(size_t)s*W;
    for(int k=0;k<nn;k++){
     int p=nbr[k];const uint64_t*tp=T+(size_t)p*W;
     for(int w=0;w<W;w++){uint64_t x=tp[w]&nm[w];while(x){int bb=__builtin_ctzll(x);x&=x-1;if(nb<4)bl[nb]=(uint64_t)p*NS+w*64+bb;nb++;}}
    }
    if(nb!=4)continue;
    for(int i=0;i<4;i++)for(int j=i+1;j<4;j++)if(bl[j]<bl[i]){uint64_t t=bl[i];bl[i]=bl[j];bl[j]=t;}
    if(ln<100000){lk[ln]=(u128)bl[0]|((u128)bl[1]<<29)|((u128)bl[2]<<58)|((u128)bl[3]<<87);lp[ln]=(uint32_t)idx;ln++;}
   }
  }
  #pragma omp critical
  { for(long i=0;i<ln&&n4<cap4;i++){k4[n4]=lk[i];p4[n4]=lp[i];n4++;} }
  free(lk);free(lp);
 }
 fprintf(stderr,"m4 %ld\n",n4);
 Tab quad,one,pair,three;
 tab_init(&quad,1<<22);tab_init(&one,1<<16);tab_init(&pair,1<<19);tab_init(&three,1<<21);
 for(long i=0;i<n4;i++){Ent*e=tab_put(&quad,k4[i]);if(e->n<8)e->pts[e->n++]=p4[i];}
 FILE*bf=fopen("/tmp/p004_blockers.txt","r");
 if(!bf){perror("blockers");return 1;}
 char line[256];
 Rec128 *thirds=malloc(sizeof(Rec128)*4000000); long nth=0;
 Rec64 *parts=malloc(sizeof(Rec64)*400000); long npt=0;
 while(fgets(line,sizeof line,bf)){
  int m; unsigned x; unsigned long long b1,b2,b3;
  int k=sscanf(line,"%d %u %llu %llu %llu",&m,&x,&b1,&b2,&b3);
  if(k<3)continue;
  if(m==1){Ent*e=tab_put(&one,(u128)b1);if(e->n<8)e->pts[e->n++]=x;}
  else if(m==2){
   uint64_t a1=b1<b2?b1:b2,a2=b1<b2?b2:b1;
   Ent*e=tab_put(&pair,(u128)a1|((u128)a2<<29));if(e->n<8)e->pts[e->n++]=x;
   parts[npt].key=a1|(a2<<29);parts[npt].w=(uint32_t)a2;npt++;
   parts[npt].key=a2|(a1<<29);parts[npt].w=(uint32_t)a1;npt++;
  } else if(m==3){
   unsigned long long v[3]={b1,b2,b3};
   for(int i=0;i<3;i++)for(int j=i+1;j<3;j++)if(v[j]<v[i]){unsigned long long t=v[i];v[i]=v[j];v[j]=t;}
   Ent*e=tab_put(&three,(u128)v[0]|((u128)v[1]<<29)|((u128)v[2]<<58));if(e->n<8)e->pts[e->n++]=x;
   uint64_t p01=v[0]|(v[1]<<29),p02=v[0]|(v[2]<<29),p12=v[1]|(v[2]<<29);
   thirds[nth].key=(u128)p01;thirds[nth].w=(uint32_t)v[2];nth++;
   thirds[nth].key=(u128)p02;thirds[nth].w=(uint32_t)v[1];nth++;
   thirds[nth].key=(u128)p12;thirds[nth].w=(uint32_t)v[0];nth++;
  }
 }
 fclose(bf);
 qsort(thirds,nth,sizeof(Rec128),cmp128);
 qsort(parts,npt,sizeof(Rec64),cmp64);
 // fourths index from the quad map: for each quad {a,b,c,d}, 4 records (triple key -> fourth)
 Rec128 *fourths=malloc(sizeof(Rec128)*12000000); long nfo=0;
 for(uint64_t i=0;i<=quad.mask;i++){
  if(!quad.e[i].key)continue;
  u128 key=quad.e[i].key-1;
  uint32_t v[4]={(uint32_t)(key&MASK29),(uint32_t)((key>>29)&MASK29),(uint32_t)((key>>58)&MASK29),(uint32_t)((key>>87)&MASK29)};
  for(int r=0;r<4;r++){
   uint32_t w=v[r],t3[3];int k=0;
   for(int j=0;j<4;j++)if(j!=r)t3[k++]=v[j];
   fourths[nfo].key=(u128)t3[0]|((u128)t3[1]<<29)|((u128)t3[2]<<58);fourths[nfo].w=w;nfo++;
  }
 }
 qsort(fourths,nfo,sizeof(Rec128),cmp128);
 long nq=0;for(uint64_t i=0;i<=quad.mask;i++)if(quad.e[i].key)nq++;
 fprintf(stderr,"quads %ld thirds %ld fourths %ld parts %ld\n",nq,nth,nfo,npt);
 long checked=0,found=0;long maxpool=0,maxcand=0;
 #pragma omp parallel for schedule(dynamic,256) reduction(+:checked,found)
 for(uint64_t i=0;i<=quad.mask;i++){
  if(!quad.e[i].key)continue;
  u128 key=quad.e[i].key-1;
  uint32_t d[4]={(uint32_t)(key&MASK29),(uint32_t)((key>>29)&MASK29),(uint32_t)((key>>58)&MASK29),(uint32_t)((key>>87)&MASK29)};
  // base pool
  uint32_t base[48];int nb=0;
  for(int j=0;j<quad.e[i].n;j++)base[nb++]=quad.e[i].pts[j];
  for(int r=0;r<4;r++){
   uint32_t t3[3];int k=0;for(int j=0;j<4;j++)if(j!=r)t3[k++]=d[j];
   Ent*e=tab_find(&three,(u128)t3[0]|((u128)t3[1]<<29)|((u128)t3[2]<<58));
   if(e)for(int t=0;t<e->n;t++)if(nb<48)base[nb++]=e->pts[t];
  }
  for(int i1=0;i1<4;i1++)for(int i2=i1+1;i2<4;i2++){
   Ent*e=tab_find(&pair,(u128)d[i1]|((u128)d[i2]<<29));
   if(e)for(int t=0;t<e->n;t++)if(nb<48)base[nb++]=e->pts[t];
  }
  for(int j=0;j<4;j++){Ent*e=tab_find(&one,(u128)d[j]);if(e)for(int t=0;t<e->n;t++)if(nb<48)base[nb++]=e->pts[t];}
  // candidates
  uint32_t cand[512];int nc=0;
  for(int r=0;r<4;r++){
   uint32_t t3[3];int k=0;for(int j=0;j<4;j++)if(j!=r)t3[k++]=d[j];
   u128 kk=(u128)t3[0]|((u128)t3[1]<<29)|((u128)t3[2]<<58);
   long lo=lb128(fourths,nfo,kk);
   for(long t=lo;t<nfo&&fourths[t].key==kk;t++){uint32_t w=fourths[t].w;
    int bad=0;for(int z=0;z<4;z++)if(w==d[z])bad=1;
    if(!bad&&nc<512)cand[nc++]=w;}
  }
  for(int i1=0;i1<4;i1++)for(int i2=i1+1;i2<4;i2++){
   u128 kk=(u128)d[i1]|((u128)d[i2]<<29);
   long lo=lb128(thirds,nth,kk);
   for(long t=lo;t<nth&&thirds[t].key==kk;t++){uint32_t w=thirds[t].w;
    int bad=0;for(int z=0;z<4;z++)if(w==d[z])bad=1;
    if(!bad&&nc<512)cand[nc++]=w;}
  }
  for(int j=0;j<4;j++){
   long lo=lb64(parts,npt,(uint64_t)d[j]);
   for(long t=lo;t<npt&&parts[t].key==(uint64_t)d[j];t++){uint32_t w=parts[t].w;
    int bad=0;for(int z=0;z<4;z++)if(w==d[z])bad=1;
    if(!bad&&nc<512)cand[nc++]=w;}
  }
  int nc2=0;for(int j=0;j<nc;j++){int dup=0;for(int k2=0;k2<nc2;k2++)if(cand[k2]==cand[j]){dup=1;break;}if(!dup)cand[nc2++]=cand[j];}
  if(nc2>maxcand)maxcand=nc2;
  for(int ci=0;ci<nc2;ci++){
   uint32_t e=cand[ci];
   uint32_t pool[64];int np=nb;
   memcpy(pool,base,nb*4);
   {Ent*z=tab_find(&one,(u128)e);if(z)for(int t=0;t<z->n;t++)if(np<64)pool[np++]=z->pts[t];}
   for(int j=0;j<4;j++){
    uint32_t v[2]={e<d[j]?e:d[j],e<d[j]?d[j]:e};
    Ent*z=tab_find(&pair,(u128)v[0]|((u128)v[1]<<29));if(z)for(int t=0;t<z->n;t++)if(np<64)pool[np++]=z->pts[t];
   }
   for(int i1=0;i1<4;i1++)for(int i2=i1+1;i2<4;i2++){
    uint32_t v[3]={e,d[i1],d[i2]};sort3(v);
    Ent*z=tab_find(&three,(u128)v[0]|((u128)v[1]<<29)|((u128)v[2]<<58));if(z)for(int t=0;t<z->n;t++)if(np<64)pool[np++]=z->pts[t];
   }
   for(int r=0;r<4;r++){
    uint32_t t3[3];int k=0;for(int j=0;j<4;j++)if(j!=r)t3[k++]=d[j];
    uint32_t vv[4]={e,t3[0],t3[1],t3[2]};sort4(vv);
    Ent*z=tab_find(&quad,(u128)vv[0]|((u128)vv[1]<<29)|((u128)vv[2]<<58)|((u128)vv[3]<<87));
    if(z)for(int t=0;t<z->n;t++)if(np<64)pool[np++]=z->pts[t];
   }
   if(np>maxpool)maxpool=np;
   if(np<6)continue;
   checked++;
   uint64_t am[64];
   for(int x=0;x<np;x++){am[x]=0;for(int y=0;y<np;y++)if(y!=x&&adj10(pool[x],pool[y]))am[x]|=1ULL<<y;}
   uint64_t fullm=(np>=64)?~0ULL:((1ULL<<np)-1);
   if(hasrec(fullm,6,am)){found++;printf("FOUND d %u %u %u %u e %u np %d\n",d[0],d[1],d[2],d[3],e,np);}
  }
 }
 printf("quads %ld checked %ld found %ld maxpool %ld maxcand %ld\n",nq,checked,found,maxpool,maxcand);
 return 0;
}
