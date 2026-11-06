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

typedef struct { u128 key; uint32_t pts[8]; uint8_t n; } Ent;
typedef struct { Ent *e; uint64_t mask; } Tab;

static inline uint64_t h64(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;}
static inline uint64_t h128(u128 x){uint64_t lo=(uint64_t)x,hi=(uint64_t)(x>>64);return h64(lo^(hi*0x9E3779B97F4A7C15ULL));}
static void tab_init(Tab*t,uint64_t sz){t->e=calloc(sz,sizeof(Ent));if(!t->e){fprintf(stderr,"oom\n");exit(1);}t->mask=sz-1;}
static Ent* tab_find(Tab*t,u128 key){uint64_t h=h128(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}return NULL;}
static Ent* tab_put(Tab*t,u128 key){uint64_t h=h128(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}t->e[h].key=key+1;return &t->e[h];}

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
int main(int argc,char**argv){
 int mode=argc>1?atoi(argv[1]):0;
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
 long hist[12]={0};
 for(long i=0;i<(long)N;i++){int v=mm[i];if(v<12)hist[v]++;}
 printf("hist:");for(int i=0;i<12;i++)printf(" %d:%ld",i,hist[i]);printf("\n");fflush(stdout);
 if(mode==0)return 0;
 long cap=3000000;
 u128 *keys=malloc(sizeof(u128)*cap); uint32_t *pts=malloc(cap*4); long cnt=0;
 double t0=omp_get_wtime();
 #pragma omp parallel
 {
  u128 *lk=malloc(sizeof(u128)*200000); uint32_t *lp=malloc(200000*4); long ln=0;
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
    u128 key=(u128)bl[0]|((u128)bl[1]<<29)|((u128)bl[2]<<58)|((u128)bl[3]<<87);
    if(ln<200000){lk[ln]=key;lp[ln]=(uint32_t)idx;ln++;}
   }
  }
  #pragma omp critical
  { for(long i=0;i<ln;i++){if(cnt<cap){keys[cnt]=lk[i];pts[cnt]=lp[i];cnt++;}} }
  free(lk);free(lp);
 }
 fprintf(stderr,"m4 records %ld time %.1f\n",cnt,omp_get_wtime()-t0);
 Tab quad; tab_init(&quad,1<<22);
 for(long i=0;i<cnt;i++){Ent*e=tab_put(&quad,keys[i]);if(e->n<8)e->pts[e->n++]=pts[i];}
 long nd=0;for(uint64_t i=0;i<=quad.mask;i++)if(quad.e[i].key)nd++;
 fprintf(stderr,"distinct D %ld\n",nd);
 Tab one,pair,three;
 tab_init(&one,1<<16);tab_init(&pair,1<<19);tab_init(&three,1<<21);
 FILE*bf=fopen("/tmp/p004_blockers.txt","r");
 if(!bf){perror("blockers");return 1;}
 char line[256];long nl=0;
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
  nl++;
 }
 fclose(bf);fprintf(stderr,"blockers lines %ld\n",nl);
 long checked=0,found=0;long maxpool=0;
 #pragma omp parallel for schedule(dynamic,4096) reduction(+:checked,found)
 for(uint64_t i=0;i<=quad.mask;i++){
  if(!quad.e[i].key)continue;
  u128 key=quad.e[i].key-1;
  uint32_t a=(uint32_t)(key&MASK29),b=(uint32_t)((key>>29)&MASK29),c=(uint32_t)((key>>58)&MASK29),d=(uint32_t)((key>>87)&MASK29);
  uint32_t pool[64];int npool=0;
  for(int j=0;j<quad.e[i].n;j++)pool[npool++]=quad.e[i].pts[j];
  uint32_t tri[4][3]={{a,b,c},{a,b,d},{a,c,d},{b,c,d}};
  for(int j=0;j<4;j++){
   u128 kk=(u128)tri[j][0]|((u128)tri[j][1]<<29)|((u128)tri[j][2]<<58);
   Ent*e=tab_find(&three,kk);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];
  }
  uint32_t pr[6][2]={{a,b},{a,c},{a,d},{b,c},{b,d},{c,d}};
  for(int j=0;j<6;j++){
   u128 kk=(u128)pr[j][0]|((u128)pr[j][1]<<29);
   Ent*e=tab_find(&pair,kk);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];
  }
  uint32_t sg[4]={a,b,c,d};
  for(int j=0;j<4;j++){Ent*e=tab_find(&one,(u128)sg[j]);if(e)for(int t=0;t<e->n;t++)if(npool<64)pool[npool++]=e->pts[t];}
  if(npool>maxpool)maxpool=npool;
  if(npool<5)continue;
  checked++;
  int hit=0;
  for(int i1=0;i1<npool&&!hit;i1++)
  for(int i2=i1+1;i2<npool&&!hit;i2++){
   if(adj10(pool[i1],pool[i2]))continue;
   for(int i3=i2+1;i3<npool&&!hit;i3++){
    if(adj10(pool[i1],pool[i3])||adj10(pool[i2],pool[i3]))continue;
    for(int i4=i3+1;i4<npool&&!hit;i4++){
     if(adj10(pool[i1],pool[i4])||adj10(pool[i2],pool[i4])||adj10(pool[i3],pool[i4]))continue;
     for(int i5=i4+1;i5<npool&&!hit;i5++){
      if(adj10(pool[i1],pool[i5])||adj10(pool[i2],pool[i5])||adj10(pool[i3],pool[i5])||adj10(pool[i4],pool[i5]))continue;
      hit=1;
      printf("FOUND D %u %u %u %u pts %u %u %u %u %u\n",a,b,c,d,pool[i1],pool[i2],pool[i3],pool[i4],pool[i5]);
      break;
     }
    }
   }
  }
  if(hit)found++;
 }
 printf("checked %ld found %ld maxpool %ld\n",checked,found,maxpool);
 return 0;
}
