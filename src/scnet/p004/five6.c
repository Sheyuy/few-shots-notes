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
// ---- tables ----
typedef struct { u128 key; uint32_t pts[8]; uint8_t n; } Ent;       // exact key (up to 4 fields)
typedef struct { Ent *e; uint64_t mask; } Tab;
static inline uint64_t h64(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;}
static inline uint64_t h128(u128 x){uint64_t lo=(uint64_t)x,hi=(uint64_t)(x>>64);return h64(lo^(hi*0x9E3779B97F4A7C15ULL));}
static void tab_init(Tab*t,uint64_t sz){t->e=calloc(sz,sizeof(Ent));if(!t->e){fprintf(stderr,"oom tab\n");exit(1);}t->mask=sz-1;}
static Ent* tab_find(Tab*t,u128 key){uint64_t h=h128(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}return NULL;}
static Ent* tab_put(Tab*t,u128 key){uint64_t h=h128(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}t->e[h].key=key+1;return &t->e[h];}

typedef struct { uint64_t hash; uint32_t bl[5]; uint32_t pts[8]; uint8_t n; } Ent5;
static Ent5 *five; static uint64_t fivemask;
static void five_init(uint64_t sz){five=calloc(sz,sizeof(Ent5));if(!five){fprintf(stderr,"oom five\n");exit(1);}fivemask=sz-1;}
static inline uint64_t hash5(const uint32_t b[5]){
 uint64_t x=0x9E3779B97F4A7C15ULL;
 for(int i=0;i<5;i++){x^=(uint64_t)b[i]*0x9E3779B97F4A7C15ULL;x=(x<<31)|(x>>33);x*=0xBF58476D1CE4E5B9ULL;}
 return x;
}
static Ent5* five_findput(const uint32_t bl[5],int create){
 uint64_t h=hash5(bl)+1; uint64_t p=h&fivemask;
 while(five[p].hash){
  if(five[p].hash==h&&memcmp(five[p].bl,bl,20)==0)return &five[p];
  p=(p+1)&fivemask;
 }
 if(!create)return NULL;
 five[p].hash=h; memcpy(five[p].bl,bl,20);
 return &five[p];
}

static int hasrec(uint64_t avail,int need,uint64_t *am){
 if(need==0)return 1;
 if(__builtin_popcountll(avail)<need)return 0;
 int v=__builtin_ctzll(avail);
 uint64_t rest=avail&~(1ULL<<v);
 uint64_t av2=rest&~am[v]&~((v==63)?0ULL:((1ULL<<(v+1))-1));
 if(hasrec(av2,need-1,am))return 1;
 return hasrec(rest,need,am);
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
 if(!a||!b){fprintf(stderr,"oom conv\n");return 1;}
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
 // collect m=4 and m=5 records
 long cap4=3000000,cap5=4000000;
 u128 *k4=malloc(sizeof(u128)*cap4); uint32_t *p4=malloc(cap4*4); long n4=0;
 uint32_t (*bl5)[5]=malloc(sizeof(uint32_t)*5*cap5); long n5=0;
 uint32_t *pt5=malloc(cap5*4);
 double t0=omp_get_wtime();
 #pragma omp parallel
 {
  u128 *lk=malloc(sizeof(u128)*100000); uint32_t *lp=malloc(100000*4); long ln=0;
  uint32_t (*l5)[5]=malloc(sizeof(uint32_t)*5*100000); uint32_t *l5p=malloc(100000*4); long ln5=0;
  #pragma omp for schedule(dynamic,8)
  for(int q=0;q<NP;q++){
   int nbr[243],nn=0;const uint64_t*mq=Nmask+(size_t)q*W;
   for(int w=0;w<W;w++){uint64_t x=mq[w];while(x){int bb=__builtin_ctzll(x);x&=x-1;int p=w*64+bb;if(gcnt[p])nbr[nn++]=p;}}
   for(int s=0;s<NS;s++){
    uint64_t idx=(uint64_t)q*NS+s;
    int m=mm[idx];
    if(m!=4&&m!=5)continue;
    if((T[(size_t)q*W+(s>>6)]>>(s&63))&1)continue;
    uint64_t bl[8];int nb=0;const uint64_t*nm=Nmask+(size_t)s*W;
    for(int k=0;k<nn;k++){
     int p=nbr[k];const uint64_t*tp=T+(size_t)p*W;
     for(int w=0;w<W;w++){uint64_t x=tp[w]&nm[w];while(x){int bb=__builtin_ctzll(x);x&=x-1;if(nb<8)bl[nb]=(uint64_t)p*NS+w*64+bb;nb++;}}
    }
    if(nb!=m)continue;
    for(int i=0;i<nb;i++)for(int j=i+1;j<nb;j++)if(bl[j]<bl[i]){uint64_t t=bl[i];bl[i]=bl[j];bl[j]=t;}
    if(m==4){if(ln<100000){lk[ln]=(u128)bl[0]|((u128)bl[1]<<29)|((u128)bl[2]<<58)|((u128)bl[3]<<87);lp[ln]=(uint32_t)idx;ln++;}}
    else{if(ln5<100000){uint32_t b5[5];for(int i=0;i<5;i++)b5[i]=(uint32_t)bl[i];
      for(int i=0;i<5;i++)for(int j=i+1;j<5;j++)if(b5[j]<b5[i]){uint32_t t=b5[i];b5[i]=b5[j];b5[j]=t;}
      memcpy(l5[ln5],b5,20);l5p[ln5]=(uint32_t)idx;ln5++;}}
   }
  }
  #pragma omp critical
  {
   for(long i=0;i<ln&&n4<cap4;i++){k4[n4]=lk[i];p4[n4]=lp[i];n4++;}
   for(long i=0;i<ln5&&n5<cap5;i++){memcpy(bl5[n5],l5[i],20);pt5[n5]=l5p[i];n5++;}
  }
  free(lk);free(lp);free(l5);free(l5p);
 }
 fprintf(stderr,"m4 %ld m5 %ld scan %.1fs\n",n4,n5,omp_get_wtime()-t0);
 Tab quad,one,pair,three;
 tab_init(&quad,1<<22);tab_init(&one,1<<16);tab_init(&pair,1<<19);tab_init(&three,1<<21);
 for(long i=0;i<n4;i++){Ent*e=tab_put(&quad,k4[i]);if(e->n<8)e->pts[e->n++]=p4[i];}
 five_init(1<<23);
 long cap5pts=0;
 for(long i=0;i<n5;i++){Ent5*e=five_findput(bl5[i],1);if(e->n<8){e->pts[e->n++]=pt5[i];}else cap5pts++;}
 long nd5=0;for(uint64_t i=0;i<=fivemask;i++)if(five[i].hash)nd5++;
 fprintf(stderr,"five D %ld capped %ld\n",nd5,cap5pts);
 FILE*bf=fopen("/tmp/p004_blockers.txt","r");
 if(!bf){perror("blockers");return 1;}
 char line[256];
 while(fgets(line,sizeof line,bf)){
  int m; unsigned x; unsigned long long b1,b2,b3;
  int k=sscanf(line,"%d %u %llu %llu %llu",&m,&x,&b1,&b2,&b3);
  if(k<3)continue;
  if(m==1){Ent*e=tab_put(&one,(u128)b1);if(e->n<8)e->pts[e->n++]=x;}
  else if(m==2){
   uint64_t a1=b1<b2?b1:b2,a2=b1<b2?b2:b1;
   Ent*e=tab_put(&pair,(u128)a1|((u128)a2<<29));if(e->n<8)e->pts[e->n++]=x;
  } else if(m==3){
   unsigned long long v[3]={b1,b2,b3};
   for(int i=0;i<3;i++)for(int j=i+1;j<3;j++)if(v[j]<v[i]){unsigned long long t=v[i];v[i]=v[j];v[j]=t;}
   Ent*e=tab_put(&three,(u128)v[0]|((u128)v[1]<<29)|((u128)v[2]<<58));if(e->n<8)e->pts[e->n++]=x;
  }
 }
 fclose(bf);
 long checked=0,found=0;long maxpool=0;
 #pragma omp parallel for schedule(dynamic,256) reduction(+:checked,found)
 for(uint64_t i=0;i<=fivemask;i++){
  if(!five[i].hash)continue;
  uint32_t *d=five[i].bl;
  uint32_t pool[64];int npool=0;
  for(int j=0;j<five[i].n;j++)pool[npool++]=five[i].pts[j];
  // 4-subsets
  for(int r=0;r<5;r++){
   uint32_t v[4];int k=0;
   for(int j=0;j<5;j++)if(j!=r)v[k++]=d[j];
   u128 kk=(u128)v[0]|((u128)v[1]<<29)|((u128)v[2]<<58)|((u128)v[3]<<87);
   Ent*e=tab_find(&quad,kk);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];
  }
  // 3-subsets
  for(int i1=0;i1<5;i1++)for(int i2=i1+1;i2<5;i2++)for(int i3=i2+1;i3<5;i3++){
   u128 kk=(u128)d[i1]|((u128)d[i2]<<29)|((u128)d[i3]<<58);
   Ent*e=tab_find(&three,kk);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];
  }
  // 2-subsets
  for(int i1=0;i1<5;i1++)for(int i2=i1+1;i2<5;i2++){
   u128 kk=(u128)d[i1]|((u128)d[i2]<<29);
   Ent*e=tab_find(&pair,kk);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];
  }
  for(int j=0;j<5;j++){Ent*e=tab_find(&one,(u128)d[j]);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];}
  if(npool>maxpool)maxpool=npool;
  if(npool<6)continue;
  checked++;
  uint64_t am[64];
  for(int x=0;x<npool;x++){am[x]=0;for(int y=0;y<npool;y++)if(y!=x&&adj10(pool[x],pool[y]))am[x]|=1ULL<<y;}
  uint64_t full=(npool==64)?~0ULL:((1ULL<<npool)-1);
  if(hasrec(full,6,am)){
   found++;
   printf("FOUND d %u %u %u %u %u npool %d\n",d[0],d[1],d[2],d[3],d[4],npool);
  }
 }
 printf("fiveD %ld checked %ld found %ld maxpool %ld\n",nd5,checked,found,maxpool);
 return 0;
}
