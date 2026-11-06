#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#define NP 16807
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

static int cmpu32(const void*a,const void*b){uint32_t x=*(uint32_t*)a,y=*(uint32_t*)b;return x<y?-1:(x>y?1:0);}
static int has5rec(uint64_t avail,int need,uint64_t *am){
 if(need==0)return 1;
 if(__builtin_popcountll(avail)<need)return 0;
 int v=__builtin_ctzll(avail);
 uint64_t rest=avail&~(1ULL<<v);
 // include v
 uint64_t av2=rest&~am[v]&~((v==63)?0ULL:((1ULL<<(v+1))-1));
 if(has5rec(av2,need-1,am))return 1;
 return has5rec(rest,need,am);
}

int main(void){
 Tab one,pair;
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
 // build adjacency from pair keys
 long ne=0;for(uint64_t i=0;i<=pair.mask;i++)if(pair.e[i].key)ne++;
 fprintf(stderr,"edges %ld\n",ne);
 uint32_t *eu=malloc(sizeof(uint32_t)*ne),*ev=malloc(sizeof(uint32_t)*ne);
 long k2=0;
 for(uint64_t i=0;i<=pair.mask;i++){
  if(!pair.e[i].key)continue;
  u128 key=pair.e[i].key-1;
  eu[k2]=(uint32_t)(key&MASK29);ev[k2]=(uint32_t)((key>>29)&MASK29);k2++;
 }
 // degree
 // use a simple sorted edge list per vertex via arrays: count degrees first
 // vertices up to 282M -> use hash map from vertex to index? instead build adjacency by sorting edges
 // Build CSR over the distinct vertices appearing in edges.
 uint32_t *verts=malloc(sizeof(uint32_t)*2*ne);
 for(long i=0;i<ne;i++){verts[2*i]=eu[i];verts[2*i+1]=ev[i];}
 qsort(verts,2*ne,sizeof(uint32_t),cmpu32);
 long nv=0;for(long i=0;i<2*ne;i++)if(i==0||verts[i]!=verts[i-1])verts[nv++]=verts[i];
 fprintf(stderr,"vertices %ld\n",nv);
 // map vertex -> index via binary search
 long *deg=calloc(nv,sizeof(long));
 for(long i=0;i<ne;i++){
  long lo=0,hi=nv;while(lo<hi){long m=(lo+hi)/2;if(verts[m]<eu[i])lo=m+1;else hi=m;}deg[lo]++;
  lo=0;hi=nv;while(lo<hi){long m=(lo+hi)/2;if(verts[m]<ev[i])lo=m+1;else hi=m;}deg[lo]++;
 }
 long *off=malloc(sizeof(long)*(nv+1));off[0]=0;
 for(long i=0;i<nv;i++)off[i+1]=off[i]+deg[i];
 uint32_t *adj=malloc(sizeof(uint32_t)*2*ne);
 long *fill=calloc(nv,sizeof(long));
 for(long i=0;i<ne;i++){
  long lo=0,hi=nv;while(lo<hi){long m=(lo+hi)/2;if(verts[m]<eu[i])lo=m+1;else hi=m;}long u=lo;
  lo=0;hi=nv;while(lo<hi){long m=(lo+hi)/2;if(verts[m]<ev[i])lo=m+1;else hi=m;}long v=lo;
  adj[off[u]+fill[u]++]=ev[i];adj[off[v]+fill[v]++]=eu[i];
 }
 free(fill);free(deg);
 // enumerate P4 and stars, dedup D via hash set
 Tab ds; tab_init(&ds,1<<20);
 #pragma omp parallel
 {
  u128 *lk=malloc(sizeof(u128)*200000);long ln=0;
  #pragma omp for schedule(dynamic,64)
  for(long i=0;i<ne;i++){
   // find indices of eu[i],ev[i]
   uint32_t b=eu[i],c=ev[i];
   long lb=0,hi=nv;while(lb<hi){long m=(lb+hi)/2;if(verts[m]<b)lb=m+1;else hi=m;}
   long lc=0,hi2=nv;while(lc<hi2){long m=(lc+hi2)/2;if(verts[m]<c)lc=m+1;else hi2=m;}
   for(long ia=off[lb];ia<off[lb+1];ia++){
    uint32_t a=adj[ia];if(a==c)continue;
    for(long id=off[lc];id<off[lc+1];id++){
     uint32_t d=adj[id];if(d==a||d==b)continue;
     uint32_t v[4]={a,b,c,d};
     for(int x=0;x<4;x++)for(int y=x+1;y<4;y++)if(v[y]<v[x]){uint32_t t=v[x];v[x]=v[y];v[y]=t;}
     u128 key=(u128)v[0]|((u128)v[1]<<29)|((u128)v[2]<<58)|((u128)v[3]<<87);
     if(ln<200000)lk[ln++]=key;
    }
   }
  }
  #pragma omp critical
  { for(long i=0;i<ln;i++)tab_put(&ds,lk[i]); }
  free(lk);
 }
 // stars
 for(long b=0;b<nv;b++){
  long dg=off[b+1]-off[b];
  if(dg<3)continue;
  for(long i=0;i<dg;i++)for(long j=i+1;j<dg;j++)for(long k=j+1;k<dg;k++){
   uint32_t v[4]={verts[b],adj[off[b]+i],adj[off[b]+j],adj[off[b]+k]};
   for(int x=0;x<4;x++)for(int y=x+1;y<4;y++)if(v[y]<v[x]){uint32_t t=v[x];v[x]=v[y];v[y]=t;}
   u128 key=(u128)v[0]|((u128)v[1]<<29)|((u128)v[2]<<58)|((u128)v[3]<<87);
   tab_put(&ds,key);
  }
 }
 long nd=0;for(uint64_t i=0;i<=ds.mask;i++)if(ds.e[i].key)nd++;
 fprintf(stderr,"distinct D %ld\n",nd);
 long checked=0,found=0;long maxpool=0;
 #pragma omp parallel for schedule(dynamic,1024) reduction(+:checked,found)
 for(uint64_t i=0;i<=ds.mask;i++){
  if(!ds.e[i].key)continue;
  u128 key=ds.e[i].key-1;
  uint32_t a=(uint32_t)(key&MASK29),b=(uint32_t)((key>>29)&MASK29),c=(uint32_t)((key>>58)&MASK29),d=(uint32_t)((key>>87)&MASK29);
  uint32_t pool[32];int np=0;
  uint32_t pr[6][2]={{a,b},{a,c},{a,d},{b,c},{b,d},{c,d}};
  for(int j=0;j<6;j++){
   uint32_t u=pr[j][0],v=pr[j][1];if(u>v){uint32_t t=u;u=v;v=t;}
   Ent*e=tab_find(&pair,(u128)u|((u128)v<<29));if(e)for(int t=0;t<e->n;t++)if(np<32)pool[np++]=e->pts[t];
  }
  uint32_t sg[4]={a,b,c,d};
  for(int j=0;j<4;j++){Ent*e=tab_find(&one,(u128)sg[j]);if(e)for(int t=0;t<e->n;t++)if(np<32)pool[np++]=e->pts[t];}
  if(np>maxpool)maxpool=np;
  if(np<5)continue;
  checked++;
  uint64_t am[32];
  for(int x=0;x<np;x++){am[x]=0;for(int y=0;y<np;y++)if(y!=x&&adj10(pool[x],pool[y]))am[x]|=1ULL<<y;}
  uint64_t full=(np==64)?~0ULL:((1ULL<<np)-1);
  int hit=has5rec(full,5,am);
  if(hit){
   found++;
   printf("FOUND D %u %u %u %u pool",a,b,c,d);
   for(int x=0;x<np;x++)printf(" %u",pool[x]);
   printf("\n");
  }
 }
 printf("D %ld checked %ld found %ld maxpool %ld\n",nd,checked,found,maxpool);
 return 0;
}
