#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#define N 282475249ULL
static uint16_t *a,*b;
static int dim,nth;
static uint64_t sk,hi_count,lo_count;
static void* worker(void* arg){
    long id=(long)arg;
    for(uint64_t hi=(uint64_t)id; hi<hi_count; hi+=nth){
        uint64_t base0=hi*7*sk;
        for(uint64_t lo=0; lo<lo_count; lo++){
            uint64_t idx0=base0+lo;
            uint16_t v[7];
            for(int d=0;d<7;d++) v[d]=a[idx0+(uint64_t)d*sk];
            for(int d=0;d<7;d++) b[idx0+(uint64_t)d*sk]=(uint16_t)(v[(d+6)%7]+v[d]+v[(d+1)%7]);
        }
    }
    return NULL;
}
int main(int argc,char**argv){
    nth=argc>1?atoi(argv[1]):8;
    a=malloc(N*2); b=malloc(N*2); if(!a||!b){printf("alloc fail\n");return 1;}
    memset(a,0,N*2);
    FILE*f=fopen("/tmp/p004_points.txt","r"); char buf[32]; long np=0;
    while(fscanf(f,"%31s",buf)==1){
        uint64_t idx=0;
        for(int i=0;i<10;i++) idx=idx*7+(buf[i]-'0');
        a[idx]=1; np++;
    }
    fclose(f);
    printf("points %ld\n",np); fflush(stdout);
    for(dim=0;dim<10;dim++){
        sk=1; for(int i=0;i<9-dim;i++) sk*=7;
        hi_count=1; for(int i=0;i<dim;i++) hi_count*=7;
        lo_count=1; for(int i=0;i<9-dim;i++) lo_count*=7;
        pthread_t th[64];
        for(long t=0;t<nth;t++) pthread_create(&th[t],NULL,worker,(void*)t);
        for(int t=0;t<nth;t++) pthread_join(th[t],NULL);
        uint16_t*tmp=a; a=b; b=tmp;
        printf("dim %d done\n",dim); fflush(stdout);
    }
    // histogram
    uint64_t hist[16]={0};
    for(uint64_t i=0;i<N;i++){
        uint32_t m=a[i];
        hist[m<15?m:15]++;
    }
    for(int i=0;i<16;i++) if(hist[i]) printf("m=%d count=%llu\n",i,(unsigned long long)hist[i]);
    return 0;
}
