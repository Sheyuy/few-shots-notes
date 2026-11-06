#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define NP 16807
#define NS 16807
#define W 263
static uint64_t *Nmask,*T,*U,*B; static int *gstart,*gcnt,*glist; static uint64_t fullmask[W];
static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static inline int enc7(const int d[5]){int r=0;for(int i=0;i<5;i++)r=r*7+d[i];return r;}
static void build_neighbors(void){int d[5];for(int s=0;s<NS;s++){digits7(s,d);uint64_t*m=Nmask+(size_t)s*W;int a[5][3];
 for(int k=0;k<5;k++){a[k][0]=(d[k]+6)%7;a[k][1]=d[k];a[k][2]=(d[k]+1)%7;}
 for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
  int e[5]={a[0][i0],a[1][i1],a[2][i2],a[3][i3],a[4][i4]};int t=enc7(e);m[t>>6]|=1ULL<<(t&63);}}}
int main(int argc,char**argv){
 int rp=atoi(argv[1]), rs=atoi(argv[2]);
 Nmask=calloc((size_t)NS*W,8);T=calloc((size_t)NP*W,8);U=calloc((size_t)NP*W,8);B=calloc((size_t)NP*W,8);
 gstart=calloc(NP+1,sizeof(int));gcnt=calloc(NP,sizeof(int));
 for(int w=0;w<W;w++)fullmask[w]=~0ULL; fullmask[W-1]=(1ULL<<(NS-(W-1)*64))-1;
 build_neighbors();
 FILE*f=fopen("/tmp/p004_points.txt","r");static int pref[140000],suf[140000];int n=0;char buf[32];
 while(fscanf(f,"%31s",buf)==1){int d[10];for(int i=0;i<10;i++)d[i]=buf[i]-'0';int p=0,s=0;for(int i=0;i<5;i++)p=p*7+d[i];for(int i=5;i<10;i++)s=s*7+d[i];pref[n]=p;suf[n]=s;gcnt[p]++;n++;}
 fclose(f);
 gstart[0]=0;for(int p=0;p<NP;p++)gstart[p+1]=gstart[p]+gcnt[p];
 glist=malloc((size_t)n*sizeof(int));int*fill=calloc(NP,sizeof(int));
 for(int i=0;i<n;i++){int p=pref[i];glist[gstart[p]+fill[p]++]=suf[i];}
 for(int i=0;i<n;i++){int p=pref[i];if(p==rp&&suf[i]==rs)continue;int s=suf[i];T[(size_t)p*W+(s>>6)]|=1ULL<<(s&63);}
 for(int p=0;p<NP;p++){if(!gcnt[p])continue;uint64_t*u=U+(size_t)p*W;for(int k=gstart[p];k<gstart[p+1];k++){int s=glist[k];if(p==rp&&s==rs)continue;for(int j=0;j<W;j++)u[j]|=Nmask[(size_t)s*W+j];}}
 for(int q=0;q<NP;q++){uint64_t*b=B+(size_t)q*W;const uint64_t*m=Nmask+(size_t)q*W;for(int w=0;w<W;w++){uint64_t x=m[w];while(x){int bp=__builtin_ctzll(x);x&=x-1;int r=w*64+bp;const uint64_t*u=U+(size_t)r*W;for(int j=0;j<W;j++)b[j]|=u[j];}}}
 long total=0;
 for(int q=0;q<NP;q++){const uint64_t*b=B+(size_t)q*W;const uint64_t*t=T+(size_t)q*W;
  for(int w=0;w<W;w++){uint64_t a=(~b[w])&fullmask[w]&(~t[w]);while(a){int bit=__builtin_ctzll(a);a&=a-1;int s=w*64+bit;printf("CAND %d %d\n",q,s);total++;}}}
 printf("TOTAL %ld\n",total);
 return 0;
}
