#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#define NS 16807
#define MASK29 0x1FFFFFFFULL
typedef unsigned __int128 u128;

static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
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

typedef struct { uint64_t hash; uint32_t v[5]; } Set5;
static Set5 *set; static uint64_t setmask;
static inline uint64_t hash5(const uint32_t b[5]){
 uint64_t x=0x9E3779B97F4A7C15ULL;
 for(int i=0;i<5;i++){x^=(uint64_t)b[i]*0x9E3779B97F4A7C15ULL;x=(x<<31)|(x>>33);x*=0xBF58476D1CE4E5B9ULL;}
 return x;
}
static int set_add(const uint32_t v[5]){
 uint64_t h=hash5(v)+1;uint64_t p=h&setmask;
 while(set[p].hash){
  if(set[p].hash==h&&memcmp(set[p].v,v,20)==0)return 0;
  p=(p+1)&setmask;
 }
 set[p].hash=h;memcpy(set[p].v,v,20);return 1;
}
static int cmpu32(const void*a,const void*b){uint32_t x=*(uint32_t*)a,y=*(uint32_t*)b;return x<y?-1:(x>y?1:0);}
static int hasrec(uint64_t avail,int need,uint64_t *am){
 if(need==0)return 1;
 if(__builtin_popcountll(avail)<need)return 0;
 int v=__builtin_ctzll(avail);
 uint64_t rest=avail&~(1ULL<<v);
 uint64_t av2=rest&~am[v]&~((v==63)?0ULL:((1ULL<<(v+1))-1));
 if(hasrec(av2,need-1,am))return 1;
 return hasrec(rest,need,am);
}
static Tab one,pair;
static uint32_t *verts,*adj;static long *off;static long nv;
static uint32_t *eu,*ev;static long ne;
static long vidx(uint32_t x){long lo=0,hi=nv;while(lo<hi){long m=(lo+hi)/2;if(verts[m]<x)lo=m+1;else hi=m;}return (verts[lo]==x)?lo:-1;}

int main(void){
 tab_init(&one,1<<16);tab_init(&pair,1<<19);
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
  }
 }
 fclose(bf);
 for(uint64_t i=0;i<=pair.mask;i++)if(pair.e[i].key)ne++;
 eu=malloc(sizeof(uint32_t)*ne);ev=malloc(sizeof(uint32_t)*ne);
 long k2=0;
 for(uint64_t i=0;i<=pair.mask;i++){
  if(!pair.e[i].key)continue;
  u128 key=pair.e[i].key-1;
  eu[k2]=(uint32_t)(key&MASK29);ev[k2]=(uint32_t)((key>>29)&MASK29);k2++;
 }
 uint32_t *vv=malloc(sizeof(uint32_t)*2*ne);
 for(long i=0;i<ne;i++){vv[2*i]=eu[i];vv[2*i+1]=ev[i];}
 qsort(vv,2*ne,sizeof(uint32_t),cmpu32);
 verts=malloc(sizeof(uint32_t)*2*ne);
 for(long i=0;i<2*ne;i++)if(i==0||vv[i]!=vv[i-1])verts[nv++]=vv[i];
 long *deg=calloc(nv,sizeof(long));
 for(long i=0;i<ne;i++){deg[vidx(eu[i])]++;deg[vidx(ev[i])]++;}
 off=malloc(sizeof(long)*(nv+1));off[0]=0;
 for(long i=0;i<nv;i++)off[i+1]=off[i]+deg[i];
 adj=malloc(sizeof(uint32_t)*2*ne);
 long *fill=calloc(nv,sizeof(long));
 for(long i=0;i<ne;i++){long u=vidx(eu[i]),v=vidx(ev[i]);adj[off[u]+fill[u]++]=ev[i];adj[off[v]+fill[v]++]=eu[i];}
 free(fill);free(deg);free(vv);
 fprintf(stderr,"edges %ld vertices %ld\n",ne,nv);
 set=malloc(sizeof(Set5)*(1<<23));setmask=(1<<23)-1;
 // enumerate P5 in parallel (per-thread buffers)
 long cap=20000000;
 uint32_t (*buf)[5]=malloc(sizeof(uint32_t)*5*cap); long nb=0; static long np5total=0;
 #pragma omp parallel
 {
  uint32_t (*lb)[5]=malloc(sizeof(uint32_t)*5*4000000); long ln=0; long lntot=0;
  #pragma omp for schedule(dynamic,16)
  for(long i=0;i<ne;i++){
   long bi=vidx(eu[i]),ci=vidx(ev[i]);
   for(long ia=off[bi];ia<off[bi+1];ia++){
    uint32_t a=adj[ia];if(a==ev[i])continue;
    for(long id=off[ci];id<off[ci+1];id++){
     uint32_t d=adj[id];if(d==a||d==eu[i])continue;
     long di=vidx(d);
     for(long ie=off[di];ie<off[di+1];ie++){
      uint32_t e=adj[ie];
      if(e==a||e==eu[i]||e==ev[i]||e==d)continue;
      uint32_t v[5]={a,eu[i],ev[i],d,e};
      for(int x=0;x<5;x++)for(int y=x+1;y<5;y++)if(v[y]<v[x]){uint32_t t=v[x];v[x]=v[y];v[y]=t;}
      if(ln<4000000){memcpy(lb[ln],v,20);ln++;}
      lntot++;
     }
    }
   }
  }
  #pragma omp critical
  { for(long i=0;i<ln&&nb<cap;i++){memcpy(buf[nb],lb[i],20);nb++;} np5total+=lntot; }
  free(lb);
 }
 fprintf(stderr,"p5 instances %ld (total %ld)\n",nb,np5total);
 long nd=0;
 for(long i=0;i<nb;i++)nd+=set_add(buf[i]);
 // K1,4 stars
 long nstar=0;
 for(long b=0;b<nv;b++){
  long dg=off[b+1]-off[b];
  if(dg<4)continue;
  for(long i=0;i<dg;i++)for(long j=i+1;j<dg;j++)for(long k2=j+1;k2<dg;k2++)for(long l=k2+1;l<dg;l++){
   uint32_t v[5]={verts[b],adj[off[b]+i],adj[off[b]+j],adj[off[b]+k2],adj[off[b]+l]};
   for(int x=0;x<5;x++)for(int y=x+1;y<5;y++)if(v[y]<v[x]){uint32_t t=v[x];v[x]=v[y];v[y]=t;}
   nstar+=set_add(v);
  }
 }
 long total=0;for(uint64_t i=0;i<=setmask;i++)if(set[i].hash)total++;
 fprintf(stderr,"distinct 5-sets %ld (p5 new %ld,! stars %ld)\n",total,nd,nstar);
 long checked=0,found=0;long maxpool=0;
 #pragma omp parallel for schedule(dynamic,512) reduction(+:checked,found)
 for(uint64_t i=0;i<=setmask;i++){
  if(!set[i].hash)continue;
  uint32_t *d=set[i].v;
  uint32_t pool[64];int np=0;
  for(int i1=0;i1<5;i1++)for(int i2=i1+1;i2<5;i2++){
   Ent*e=tab_find(&pair,(u128)d[i1]|((u128)d[i2]<<29));
   if(e)for(int t=0;t<e->n;t++)if(np<64)pool[np++]=e->pts[t];
  }
  for(int j=0;j<5;j++){Ent*e=tab_find(&one,(u128)d[j]);if(e)for(int t=0;t<e->n;t++)if(np<64)pool[np++]=e->pts[t];}
  if(np>maxpool)maxpool=np;
  if(np<6)continue;
  checked++;
  uint64_t am[64];
  for(int x=0;x<np;x++){am[x]=0;for(int y=0;y<np;y++)if(y!=x&&adj10(pool[x],pool[y]))am[x]|=1ULL<<y;}
  uint64_t fullm=(np>=64)?~0ULL:((1ULL<<np)-1);
  if(hasrec(fullm,6,am)){found++;printf("FOUND d %u %u %u %u %u np %d\n",d[0],d[1],d[2],d[3],d[4],np);}
 }
 printf("sets %ld checked %ld found %ld maxpool %ld\n",total,checked,found,maxpool);
 return 0;
}
