#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define H 16807            /* 7^5 */
#define NW 263             /* 16807/64 rounded up */
static uint64_t U[H][NW];
static int nb[H][243];
static unsigned char F[H];

static int pow7[5]={1,7,49,343,2401};
static int digit(int x,int k){return (x/pow7[k])%7;}
static int make(int d[5]){int r=0;for(int k=4;k>=0;k--)r=r*7+d[k];return r;}
static void initnb(void){
    for(int x=0;x<H;x++){
        int dg[5]; for(int k=0;k<5;k++)dg[k]=digit(x,k);
        int idx=0;
        for(int a=0;a<3;a++)for(int b=0;b<3;b++)for(int c=0;c<3;c++)for(int e=0;e<3;e++)for(int f=0;f<3;f++){
            int dd[5]={(dg[0]+a+2)%3,(dg[1]+b+2)%3,(dg[2]+c+2)%3,(dg[3]+e+2)%3,(dg[4]+f+2)%3};
            /* a,b,c,e,f in 0..2 represent offsets -1,0,1? use (d + off +7)%7 with off=0,1,2? */
            nb[x][idx++]=make(dd);
        }
    }
}
int main(int argc,char**argv){
    /* fix initnb: offsets should be -1,0,1 mod 7; use val = (dg + (t-1) + 7)%7 */
    for(int x=0;x<H;x++){
        int dg[5]; for(int k=0;k<5;k++)dg[k]=digit(x,k);
        int idx=0;
        for(int a=0;a<3;a++)for(int b=0;b<3;b++)for(int c=0;c<3;c++)for(int e=0;e<3;e++)for(int f=0;f<3;f++){
            int off[5]={a-1,b-1,c-1,e-1,f-1};
            int dd[5]; for(int k=0;k<5;k++) dd[k]=(dg[k]+off[k]+7)%7;
            nb[x][idx++]=make(dd);
        }
    }
    FILE*f=fopen("points.txt","r");
    long n=0; int u,v;
    while(fscanf(f,"%d %d",&u,&v)==2){
        n++;
        for(int t=0;t<243;t++){
            int uu=nb[u][t];
            U[uu][v>>6] |= 1ULL<<(v&63);
        }
    }
    fclose(f);
    printf("points %ld\n",n); fflush(stdout);
    long t0=time(NULL);
    for(int u=0;u<H;u++){
        memset(F,0,H);
        int any=0;
        for(int w=0;w<NW;w++){
            uint64_t x=U[u][w];
            while(x){
                int b=__builtin_ctzll(x); x&=x-1;
                int vv=w*64+b;
                if(vv>=H) continue;
                for(int t=0;t<243;t++) F[nb[vv][t]]=1;
                any=1;
            }
        }
        if(!any){ printf("EMPTY U at u=%d: any v works\n",u); return 0; }
        for(int vv=0;vv<H;vv++) if(!F[vv]){
            printf("HOLE u=%d v=%d\n",u,vv);
            /* output point */
            int dg[5]; for(int k=0;k<5;k++){dg[k]=digit(u,k);}
            int dh[5]; for(int k=0;k<5;k++){dh[k]=digit(vv,k);}
            printf("point: ");
            for(int k=4;k>=0;k--)printf("%d",dg[k]);
            for(int k=4;k>=0;k--)printf("%d",dh[k]);
            printf("\n");
            FILE*o=fopen("hole.txt","w"); fprintf(o,"%d %d\n",u,vv); fclose(o);
            return 0;
        }
        if((u&1023)==0) printf("u=%d t=%lds\n",u,(long)(time(NULL)-t0));
    }
    printf("no hole, maximal\n");
    return 0;
}
