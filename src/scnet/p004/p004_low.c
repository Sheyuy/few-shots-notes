#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#define N 282475249ULL
static uint16_t *a,*b; static uint8_t *ins;
static int dim,nth; static uint64_t sk,hi_count,lo_count;
static void* worker(void* arg){long id=(long)arg;
 for(uint64_t hi=(uint64_t)id;hi<hi_count;hi+=nth){uint64_t base0=hi*7*sk;
  for(uint64_t lo=0;lo<lo_count;lo++){uint64_t idx0=base0+lo;uint16_t v[7];
   for(int d=0;d<7;d++)v[d]=a[idx0+(uint64_t)d*sk];
   for(int d=0;d<7;d++)b[idx0+(uint64_t)d*sk]=(uint16_t)(v[(d+6)%7]+v[d]+v[(d+1)%7]);}}
 return NULL;}
static inline int getbit(uint64_t i){return (ins[i>>3]>>(i&7))&1;}
int main(int argc,char**argv){
 nth=argc>1?atoi(argv[1]):8;
 a=malloc(N*2);b=malloc(N*2);ins=calloc(N/8+1,1); if(!a||!b||!ins){printf("alloc fail\n");return 1;}
 memset(a,0,N*2);
 FILE*f=fopen("/tmp/p004_points.txt","r");char buf[32];long np=0;static uint64_t pts[200000];
 while(fscanf(f,"%31s",buf)==1){uint64_t idx=0;for(int i=0;i<10;i++)idx=idx*7+(buf[i]-'0');a[idx]=1;ins[idx>>3]|=1<<(idx&7);pts[np++]=idx;}
 fclose(f); printf("points %ld\n",np);fflush(stdout);
 for(dim=0;dim<10;dim++){sk=1;for(int i=0;i<9-dim;i++)sk*=7;hi_count=1;for(int i=0;i<dim;i++)hi_count*=7;lo_count=1;for(int i=0;i<9-dim;i++)lo_count*=7;
  pthread_t th[64];for(long t=0;t<nth;t++)pthread_create(&th[t],NULL,worker,(void*)t);for(int t=0;t<nth;t++)pthread_join(th[t],NULL);
  uint16_t*tmp=a;a=b;b=tmp;}
 FILE*o=fopen("/tmp/p004_low.txt","w");long n1=0,n2=0;
 uint64_t dig[10];
 for(uint64_t i=0;i<N;i++){
  if(a[i]>2)continue; if(getbit(i))continue;
  uint64_t x=i; for(int k=9;k>=0;k--){dig[k]=x%7;x/=7;}
  uint64_t bl[4];int nb=0;
  for(int m=0;m<243;m++){int t=m;uint64_t y=0;
   for(int k=0;k<10;k++){int off=t%3;t/=3;y=y*7+((dig[k]+off+6)%7);}
   if(getbit(y)){if(nb<4)bl[nb]=y;nb++;}}
  if(nb==1){fprintf(o,"1 %llu %llu\n",(unsigned long long)i,(unsigned long long)bl[0]);n1++;}
  else if(nb==2){fprintf(o,"2 %llu %llu %llu\n",(unsigned long long)i,(unsigned long long)bl[0],(unsigned long long)bl[1]);n2++;}
  else if(nb!=a[i]){printf("MISMATCH i=%llu nb=%d a=%d\n",(unsigned long long)i,nb,a[i]);}
 }
 fclose(o);printf("dumped 1-blocked=%ld 2-blocked=%ld\n",n1,n2);
 return 0;}
