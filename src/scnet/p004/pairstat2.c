#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define NP 16807
#define NS 16807
#define W 263
static uint64_t *Nmask,*U; static int *gcnt; static uint64_t fullmask[W];
static inline int enc7(const int d[5]){int r=0;for(int i=0;i<5;i++)r=r*7+d[i];return r;}
static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static void build_neighbors(void){int d[5];for(int s=0;s<NS;s++){digits7(s,d);uint64_t*m=Nmask+(size_t)s*W;int a[5][3];
 for(int k=0;k<5;k++){a[k][0]=(d[k]+6)%7;a[k][1]=d[k];a[k][2]=(d[k]+1)%7;}
 for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){int e[5]={a[0][i0],a[1][i1],a[2][i2],a[3][i3],a[4][i4]};int t=enc7(e);m[t>>6]|=1ULL<<(t&63);}}}
int main(void){
 Nmask=calloc((size_t)NS*W,8);U=calloc((size_t)NP*W,8);gcnt=calloc(NP,sizeof(int));
 for(int w=0;w<W;w++)fullmask[w]=~0ULL;fullmask[W-1]=(1ULL<<(NS-(W-1)*64))-1;
 build_neighbors();
 FILE*f=fopen("/tmp/p004_points.txt","r");char buf[32];static int pref[140000],suf[140000];int n=0;
 while(fscanf(f,"%31s",buf)==1){int d[10];for(int i=0;i<10;i++)d[i]=buf[i]-'0';int p=0,s=0;for(int i=0;i<5;i++)p=p*7+d[i];for(int i=5;i<10;i++)s=s*7+d[i];pref[n]=p;suf[n]=s;gcnt[p]++;n++;}
 fclose(f);
 for(int i=0;i<n;i++){int p=pref[i],s=suf[i];const uint64_t*m=Nmask+(size_t)s*W;uint64_t*u=U+(size_t)p*W;for(int j=0;j<W;j++)u[j]|=m[j];}
 long pairs=0; long h[10]={0}; int maxA=0;
 for(int p=0;p<NP;p++){ if(!gcnt[p]||gcnt[p]>8)continue; // only 8-groups
  const uint64_t*mp=Nmask+(size_t)p*W;
  for(int w=0;w<W;w++){uint64_t x=mp[w];while(x){int b=__builtin_ctzll(x);x&=x-1;int q=w*64+b;if(q<=p||!gcnt[q])continue;
   // A_q after removing p,q
   uint64_t A[W]; memset(A,0,(size_t)W*8);
   const uint64_t*m2=Nmask+(size_t)q*W;
   for(int w2=0;w2<W;w2++){uint64_t y=m2[w2];while(y){int b2=__builtin_ctzll(y);y&=y-1;int r=w2*64+b2;if(r==p||r==q)continue;const uint64_t*u=U+(size_t)r*W;for(int j=0;j<W;j++)A[j]|=u[j];}}
   long c=0;for(int j=0;j<W;j++)c+=__builtin_popcountll((~A[j])&fullmask[j]);
   pairs++; int g=gcnt[q]; int k=g<=8?0:(g<=20?1:(g<=26?2:(g<=341?3:(g<=347?4:(g<=359?5:6)))));
   h[k]++; if(c>maxA)maxA=c;
   if(c<40) printf("p %d (g%d) q %d (g%d) Aq %ld\n",p,gcnt[p],q,g,c);
  }}
 }
 printf("pairs %ld maxAq %d\n",pairs,maxA);
 for(int k=0;k<7;k++) if(h[k]) printf("qclass %d count %ld\n",k,h[k]);
 return 0;
}
