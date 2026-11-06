#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#define NP 16807
#define NS 16807

static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static inline int adj10(uint32_t x,uint32_t y){
 int qx=x/NS,sx=x%NS,qy=y/NS,sy=y%NS;
 int dx[5],dy[5],ex[5],ey[5];
 digits7(qx,dx);digits7(qy,dy);digits7(sx,ex);digits7(sy,ey);
 for(int i=0;i<5;i++){int e=(dx[i]-dy[i])%7;if(e>1&&e<6)return 0;}
 for(int i=0;i<5;i++){int e=(ex[i]-ey[i])%7;if(e>1&&e<6)return 0;}
 return 1;
}
typedef struct { uint64_t key; uint32_t pts[8]; uint8_t n; } Ent;
typedef struct { Ent *e; uint64_t mask; } Tab;
static inline uint64_t h64(uint64_t x){x^=x>>33;x*=0xff51afd7ed558ccdULL;x^=x>>33;x*=0xc4ceb9fe1a85ec53ULL;x^=x>>33;return x;}
static void tab_init(Tab*t,uint64_t sz){t->e=calloc(sz,sizeof(Ent));if(!t->e){fprintf(stderr,"oom\n");exit(1);}t->mask=sz-1;}
static Ent* tab_find(Tab*t,uint64_t key){uint64_t h=h64(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}return NULL;}
static Ent* tab_put(Tab*t,uint64_t key){uint64_t h=h64(key)&t->mask;while(t->e[h].key){if(t->e[h].key==key+1)return &t->e[h];h=(h+1)&t->mask;}t->e[h].key=key+1;return &t->e[h];}
typedef struct { uint64_t key; uint32_t w; } Rec;
static int cmprec(const void*a,const void*b){const Rec*x=a,*y=b;if(x->key<y->key)return -1;if(x->key>y->key)return 1;return 0;}
static long lower_bound(Rec*a,long n,uint64_t key){long lo=0,hi=n;while(lo<hi){long m=(lo+hi)/2;if(a[m].key<key)lo=m+1;else hi=m;}return lo;}

int main(void){
 Tab one,pair,three;
 tab_init(&one,1<<16);tab_init(&pair,1<<19);tab_init(&three,1<<21);
 Rec *thirds=malloc(sizeof(Rec)*4000000); long nth=0;
 Rec *parts=malloc(sizeof(Rec)*400000); long npt=0;
 FILE*bf=fopen("/tmp/p004_blockers.txt","r");
 if(!bf){perror("blockers");return 1;}
 char line[256];long nl=0;
 while(fgets(line,sizeof line,bf)){
  int m; unsigned x; unsigned long long b1,b2,b3;
  int k=sscanf(line,"%d %u %llu %llu %llu",&m,&x,&b1,&b2,&b3);
  if(k<3)continue;
  if(m==1){Ent*e=tab_put(&one,b1);if(e->n<8)e->pts[e->n++]=x;}
  else if(m==2){
   uint64_t a1=b1<b2?b1:b2,a2=b1<b2?b2:b1;
   Ent*e=tab_put(&pair,a1|(a2<<15));if(e->n<8)e->pts[e->n++]=x;
   parts[npt].key=a1;parts[npt].w=(uint32_t)a2;npt++;
   parts[npt].key=a2;parts[npt].w=(uint32_t)a1;npt++;
  } else if(m==3){
   unsigned long long v[3]={b1,b2,b3};
   for(int i=0;i<3;i++)for(int j=i+1;j<3;j++)if(v[j]<v[i]){unsigned long long t=v[i];v[i]=v[j];v[j]=t;}
   Ent*e=tab_put(&three,v[0]|(v[1]<<15)|(v[2]<<30));if(e->n<8)e->pts[e->n++]=x;
   // thirds for each pair
   uint64_t p01=v[0]|(v[1]<<15),p02=v[0]|(v[2]<<15),p12=v[1]|(v[2]<<15);
   thirds[nth].key=p01;thirds[nth].w=(uint32_t)v[2];nth++;
   thirds[nth].key=p02;thirds[nth].w=(uint32_t)v[1];nth++;
   thirds[nth].key=p12;thirds[nth].w=(uint32_t)v[0];nth++;
  }
  nl++;
 }
 fclose(bf);
 fprintf(stderr,"loaded %ld thirds %ld parts %ld\n",nl,nth,npt);
 qsort(thirds,nth,sizeof(Rec),cmprec);
 qsort(parts,npt,sizeof(Rec),cmprec);
 long ntri=0;for(uint64_t i=0;i<=three.mask;i++)if(three.e[i].key)ntri++;
 fprintf(stderr,"triples %ld\n",ntri);
 long checked=0,found=0; long maxpool=0, maxcand=0; long nz=0;
 #pragma omp parallel for schedule(dynamic,512) reduction(+:checked,found,nz)
 for(long i=0;i<=(long)three.mask;i++){
  if(!three.e[i].key)continue;
  nz++;
  uint64_t key=three.e[i].key-1;
  uint32_t a=key&32767,b=(key>>15)&32767,c=(key>>30)&32767;
  uint32_t base[24];int nb=0;
  for(int j=0;j<three.e[i].n;j++)base[nb++]=three.e[i].pts[j];
  uint32_t pr3[3][2]={{a,b},{a,c},{b,c}};
  for(int j=0;j<3;j++){uint64_t kk=(uint64_t)pr3[j][0]|((uint64_t)pr3[j][1]<<15);Ent*e=tab_find(&pair,kk);if(e)for(int t=0;t<e->n;t++)if(nb<24)base[nb++]=e->pts[t];}
  uint32_t sg3[3]={a,b,c};
  for(int j=0;j<3;j++){Ent*e=tab_find(&one,sg3[j]);if(e)for(int t=0;t<e->n;t++)if(nb<24)base[nb++]=e->pts[t];}
  // candidates e
  uint32_t cand[256];int nc=0;
  for(int j=0;j<3;j++){
   uint64_t kk=(uint64_t)pr3[j][0]|((uint64_t)pr3[j][1]<<15);
   long lo=lower_bound(thirds,nth,kk);
   for(long t=lo;t<nth&&thirds[t].key==kk;t++){uint32_t w=thirds[t].w;if(w==a||w==b||w==c)continue;if(nc<256)cand[nc++]=w;}
  }
  for(int j=0;j<3;j++){
   long lo=lower_bound(parts,npt,sg3[j]);
   for(long t=lo;t<npt&&parts[t].key==sg3[j];t++){uint32_t w=parts[t].w;if(w==a||w==b||w==c)continue;if(nc<256)cand[nc++]=w;}
  }
  // dedup cand
  int nc2=0;for(int j=0;j<nc;j++){int dup=0;for(int k=0;k<nc2;k++)if(cand[k]==cand[j]){dup=1;break;}if(!dup)cand[nc2++]=cand[j];}
  if(nc2>maxcand)maxcand=nc2;
  for(int ci=0;ci<nc2;ci++){
   uint32_t e=cand[ci];
   uint32_t pool[32];int np=nb;
   memcpy(pool,base,nb*4);
   Ent*z=tab_find(&one,e);if(z)for(int t=0;t<z->n;t++)if(np<32)pool[np++]=z->pts[t];
   uint32_t pr4[3][2]={{e,a},{e,b},{e,c}};
   for(int j=0;j<3;j++){uint64_t kk=(uint64_t)pr4[j][0]|((uint64_t)pr4[j][1]<<15);Ent*z2=tab_find(&pair,kk);if(z2)for(int t=0;t<z2->n;t++)if(np<32)pool[np++]=z2->pts[t];}
   uint32_t tr4[3][3]={{e,a,b},{e,a,c},{e,b,c}};
   for(int j=0;j<3;j++){uint64_t kk=(uint64_t)tr4[j][0]|((uint64_t)tr4[j][1]<<15)|((uint64_t)tr4[j][2]<<30);Ent*z3=tab_find(&three,kk);if(z3)for(int t=0;t<z3->n;t++)if(np<32)pool[np++]=z3->pts[t];}
   if(np>maxpool)maxpool=np;
   if(np<5)continue;
   checked++;
   int hit=0;
   for(int i1=0;i1<np&&!hit;i1++)
   for(int i2=i1+1;i2<np&&!hit;i2++){
    if(adj10(pool[i1],pool[i2]))continue;
    for(int i3=i2+1;i3<np&&!hit;i3++){
     if(adj10(pool[i1],pool[i3])||adj10(pool[i2],pool[i3]))continue;
     for(int i4=i3+1;i4<np&&!hit;i4++){
      if(adj10(pool[i1],pool[i4])||adj10(pool[i2],pool[i4])||adj10(pool[i3],pool[i4]))continue;
      for(int i5=i4+1;i5<np&&!hit;i5++){
       if(adj10(pool[i1],pool[i5])||adj10(pool[i2],pool[i5])||adj10(pool[i3],pool[i5])||adj10(pool[i4],pool[i5]))continue;
       hit=1;printf("FOUND T %u %u %u e %u pts %u %u %u %u %u\n",a,b,c,e,pool[i1],pool[i2],pool[i3],pool[i4],pool[i5]);break;
      }
     }
    }
   }
   if(hit)found++;
  }
 }
 printf("triples %ld nz %ld checked %ld found %ld maxpool %ld maxcand %ld\n",ntri,nz,checked,found,maxpool,maxcand);
 return 0;
}
