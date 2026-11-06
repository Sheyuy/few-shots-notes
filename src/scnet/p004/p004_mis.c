#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define NP 16807
#define NS 16807
#define W 263
static uint64_t *Nmask,*T,*U; static int *gstart,*gcnt,*glist; static uint64_t fullmask[W];
static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static inline int enc7(const int d[5]){int r=0;for(int i=0;i<5;i++)r=r*7+d[i];return r;}
static inline int dig(int x,int k){for(int i=4;i>k;i--)x/=7;return x%7;}
static int adjh(int a,int b){ if(a==b)return 0; for(int k=0;k<5;k++){int d=(dig(a,k)-dig(b,k)+7)%7;if(d&&d!=1&&d!=6)return 0;} return 1; }
static void build_neighbors(void){int d[5];for(int s=0;s<NS;s++){digits7(s,d);uint64_t*m=Nmask+(size_t)s*W;int a[5][3];
 for(int k=0;k<5;k++){a[k][0]=(d[k]+6)%7;a[k][1]=d[k];a[k][2]=(d[k]+1)%7;}
 for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
  int e[5]={a[0][i0],a[1][i1],a[2][i2],a[3][i3],a[4][i4]};int t=enc7(e);m[t>>6]|=1ULL<<(t&63);}}}
static int best;
static void dfs(uint32_t cand,int cnt,uint32_t *adj){
 if(cnt+__builtin_popcount(cand)<=best)return;
 if(!cand){if(cnt>best)best=cnt;return;}
 int v=__builtin_ctz(cand);
 dfs(cand&~(adj[v]|(1u<<v)),cnt+1,adj);
 dfs(cand&~(1u<<v),cnt,adj);
}
int main(void){
 Nmask=calloc((size_t)NS*W,8);T=calloc((size_t)NP*W,8);U=calloc((size_t)NP*W,8);
 gstart=calloc(NP+1,sizeof(int));gcnt=calloc(NP,sizeof(int));
 for(int w=0;w<W;w++)fullmask[w]=~0ULL;fullmask[W-1]=(1ULL<<(NS-(W-1)*64))-1;
 build_neighbors();
 FILE*f=fopen("/tmp/p004_points.txt","r");static int pref[140000],suf[140000];int n=0;char buf[32];
 while(fscanf(f,"%31s",buf)==1){int d[10];for(int i=0;i<10;i++)d[i]=buf[i]-'0';int p=0,s=0;for(int i=0;i<5;i++)p=p*7+d[i];for(int i=5;i<10;i++)s=s*7+d[i];pref[n]=p;suf[n]=s;gcnt[p]++;n++;}
 fclose(f); gstart[0]=0;for(int p=0;p<NP;p++)gstart[p+1]=gstart[p]+gcnt[p];
 glist=malloc((size_t)n*sizeof(int));int*fill=calloc(NP,sizeof(int));
 for(int i=0;i<n;i++){int p=pref[i];glist[gstart[p]+fill[p]++]=suf[i];}
 for(int i=0;i<n;i++){int p=pref[i],s=suf[i];T[(size_t)p*W+(s>>6)]|=1ULL<<(s&63);}
 for(int p=0;p<NP;p++){if(!gcnt[p])continue;uint64_t*u=U+(size_t)p*W;for(int k=gstart[p];k<gstart[p+1];k++){int s=glist[k];for(int j=0;j<W;j++)u[j]|=Nmask[(size_t)s*W+j];}}
 int improvements=0;
 for(int p=0;p<NP;p++){ if(!gcnt[p])continue; if(gcnt[p]>40)continue;
  uint64_t A[W]; memset(A,0,(size_t)W*8);
  const uint64_t*m=Nmask+(size_t)p*W;
  for(int w=0;w<W;w++){uint64_t x=m[w];while(x){int b=__builtin_ctzll(x);x&=x-1;int q=w*64+b;if(q==p)continue;const uint64_t*u=U+(size_t)q*W;for(int j=0;j<W;j++)A[j]|=u[j];}}
  int verts[64],nv=0;
  for(int w=0;w<W;w++){uint64_t a=(~A[w])&fullmask[w];while(a){int b=__builtin_ctzll(a);a&=a-1;if(nv<64)verts[nv++]=w*64+b;}}
  if(nv>32||nv<=gcnt[p])continue;
  uint32_t adj[64]; memset(adj,0,sizeof(adj));
  for(int i=0;i<nv;i++)for(int j=i+1;j<nv;j++) if(adjh(verts[i],verts[j])){adj[i]|=1u<<j;adj[j]|=1u<<i;}
  best=0; uint32_t cand=(nv==32)?0xFFFFFFFFu:((1u<<nv)-1);
  dfs(cand,0,adj);
  if(best>gcnt[p]){ printf("IMPROVE p=%d cur=%d allowed=%d mis=%d\n",p,gcnt[p],nv,best); improvements++; }
 }
 printf("improvements=%d\n",improvements);
 return 0;
}
