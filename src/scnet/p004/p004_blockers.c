#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#define NP 16807
#define NS 16807
#define W 263
#define N 282475249ULL
static uint64_t *Nmask,*T; static int *gcnt; static uint16_t *a;
static inline void digits7(int x,int d[5]){for(int i=4;i>=0;i--){d[i]=x%7;x/=7;}}
static inline int enc7(const int d[5]){int r=0;for(int i=0;i<5;i++)r=r*7+d[i];return r;}
static void build_neighbors(void){int d[5];for(int s=0;s<NS;s++){digits7(s,d);uint64_t*m=Nmask+(size_t)s*W;int a2[5][3];
 for(int k=0;k<5;k++){a2[k][0]=(d[k]+6)%7;a2[k][1]=d[k];a2[k][2]=(d[k]+1)%7;}
 for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
  int e[5]={a2[0][i0],a2[1][i1],a2[2][i2],a2[3][i3],a2[4][i4]};int t=enc7(e);m[t>>6]|=1ULL<<(t&63);}}}
int main(void){
 a=malloc(N*2); Nmask=calloc((size_t)NS*W,8); T=calloc((size_t)NP*W,8); gcnt=calloc(NP,sizeof(int));
 memset(a,0,N*2); build_neighbors();
 FILE*f=fopen("/tmp/p004_points.txt","r");char buf[32];long np=0;
 while(fscanf(f,"%31s",buf)==1){int d[10];for(int i=0;i<10;i++)d[i]=buf[i]-'0';int q=0,s=0;for(int i=0;i<5;i++)q=q*7+d[i];for(int i=5;i<10;i++)s=s*7+d[i];
  T[(size_t)q*W+(s>>6)]|=1ULL<<(s&63); gcnt[q]++; a[(uint64_t)q*NS+s]=1; np++;}
 fclose(f);
 // convolution
 uint16_t *b=malloc(N*2); memset(b,0,N*2);
 for(int dim=0;dim<10;dim++){
  uint64_t sk=1;for(int i=0;i<9-dim;i++)sk*=7;
  uint64_t hi_count=1;for(int i=0;i<dim;i++)hi_count*=7;
  uint64_t lo_count=1;for(int i=0;i<9-dim;i++)lo_count*=7;
  for(uint64_t hi=0;hi<hi_count;hi++){uint64_t base0=hi*7*sk;
   for(uint64_t lo=0;lo<lo_count;lo++){uint64_t idx0=base0+lo;uint16_t v[7];
    for(int d=0;d<7;d++)v[d]=a[idx0+(uint64_t)d*sk];
    for(int d=0;d<7;d++)b[idx0+(uint64_t)d*sk]=(uint16_t)(v[(d+6)%7]+v[d]+v[(d+1)%7]);}}
  uint16_t*tmp=a;a=b;b=tmp;}
 free(b);
 // neighbor prefix lists cached per q
 FILE*o=fopen("/tmp/p004_blockers.txt","w"); long n1=0,n2=0,nbad=0;
 int qd[5],nbr[243],nn;
 for(int q=0;q<NP;q++){
  digits7(q,qd); int t=0;
  for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
   int e[5]={(qd[0]+i0+6)%7,(qd[1]+i1+6)%7,(qd[2]+i2+6)%7,(qd[3]+i3+6)%7,(qd[4]+i4+6)%7}; nbr[t++]=enc7(e);}
  nn=243;
  for(int s=0;s<NS;s++){
   uint64_t i=(uint64_t)q*NS+s; uint16_t m=a[i];
   if(m>2)continue; if(T[(size_t)q*W+(s>>6)]>>(s&63)&1)continue;
   // find blockers
   uint64_t bl[8]; int nb=0;
   const uint64_t*nm=Nmask+(size_t)s*W;
   for(int k=0;k<nn;k++){int p=nbr[k]; if(!gcnt[p])continue; const uint64_t*tp=T+(size_t)p*W;
    for(int w=0;w<W;w++){uint64_t x=tp[w]&nm[w]; while(x){int bit=__builtin_ctzll(x);x&=x-1; if(nb<8)bl[nb]=(uint64_t)p*NS+w*64+bit; nb++;}}}
   if(nb!=m) nbad++;
   if(nb==1){fprintf(o,"1 %llu %llu\n",(unsigned long long)i,(unsigned long long)bl[0]);n1++;}
   else if(nb==2){fprintf(o,"2 %llu %llu %llu\n",(unsigned long long)i,(unsigned long long)bl[0],(unsigned long long)bl[1]);n2++;}
  }
  if(q%2000==0){printf("q=%d n1=%ld n2=%ld bad=%ld\n",q,n1,n2,nbad);fflush(stdout);}
 }
 fclose(o);printf("DONE n1=%ld n2=%ld bad=%ld\n",n1,n2,nbad);
 return 0;}
