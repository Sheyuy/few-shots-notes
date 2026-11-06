#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define H 16807
#define NW 263
static uint64_t U[H][NW];
static int nb[H][243];
static int pow7[5]={1,7,49,343,2401};
static int digit(int x,int k){return (x/pow7[k])%7;}
static int make(int d[5]){int r=0;for(int k=4;k>=0;k--)r=r*7+d[k];return r;}
int main(void){
    for(int x=0;x<H;x++){
        int dg[5]; for(int k=0;k<5;k++)dg[k]=digit(x,k);
        int idx=0;
        for(int a=0;a<3;a++)for(int b=0;b<3;b++)for(int c=0;c<3;c++)for(int e=0;e<3;e++)for(int f=0;f<3;f++){
            int off[5]={a-1,b-1,c-1,e-1,f-1}; int dd[5];
            for(int k=0;k<5;k++) dd[k]=(dg[k]+off[k]+7)%7;
            nb[x][idx++]=make(dd);
        }
    }
    FILE*f=fopen("points.txt","r"); long n=0;int u,v;
    while(fscanf(f,"%d %d",&u,&v)==2){n++; for(int t=0;t<243;t++){int uu=nb[u][t];U[uu][v>>6]|=1ULL<<(v&63);} }
    fclose(f);
    long total=0,maxc=0,minc=1<<30,empty=0;
    for(int i=0;i<H;i++){long c=0;for(int w=0;w<NW;w++)c+=__builtin_popcountll(U[i][w]);total+=c;if(c>maxc)maxc=c;if(c<minc)minc=c;if(!c)empty++;}
    printf("n=%ld total=%ld avg=%.1f max=%ld min=%ld empty=%d\n",n,total,(double)total/H,maxc,minc,empty);
    /* verify first point membership: for each neighbor uu of u, v must be in U[uu] */
    f=fopen("points.txt","r"); fscanf(f,"%d %d",&u,&v);
    int ok=1; for(int t=0;t<243;t++){int uu=nb[u][t]; if(!(U[uu][v>>6]>>(v&63)&1)) ok=0;}
    printf("membership check %d\n",ok);
    return 0;
}
