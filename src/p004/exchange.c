// p004: search for a 1-for-2 exchange.
// S is the 134753-point independent set.  Remove one point p and add two points
// q1, q2 that have p as their ONLY neighbour in S and are not adjacent to each other.
// Then |S| grows by one.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define L 16807
#define NG 243

static int *ng;          // ng[g*243 + i] : i-th neighbour group of g
static uint16_t *nl;     // nl[l*243 + i] : i-th neighbour last-key of l
static int *sstart, *Slast, *Spid;
static uint8_t *cnt;
static int32_t *owner;
static int *cand;        // up to 8 candidates per point id
static int *ccnt;

static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }

static void digits5(int idx,int d[5]){ for(int i=4;i>=0;i--){ d[i]=idx%7; idx/=7; } }

static int adjacent(long a,long b){
    for(int k=0;k<10;k++){
        int x=a%7, y=b%7;
        int d=x-y; if(d<0) d=-d;
        if(!(d==0||d==1||d==6)) return 0;
        a/=7; b/=7;
    }
    return 1;
}

int main(int argc,char**argv){
    const char *path=argv[1];
    FILE *f=fopen(path,"r");
    long n=0; long x;
    long *pts=malloc(sizeof(long)*200000);
    int *gs=malloc(sizeof(int)*200000), *ls=malloc(sizeof(int)*200000);
    while(fscanf(f,"%ld",&x)==1){
        pts[n]=x; gs[n]=x/L; ls[n]=x%L; n++;
    }
    fclose(f);
    fprintf(stderr,"loaded %ld points\n",n);

    // group the points
    sstart=calloc(L+1,sizeof(int));
    for(long i=0;i<n;i++) sstart[gs[i]+1]++;
    for(int g=0;g<L;g++) sstart[g+1]+=sstart[g];
    Slast=malloc(sizeof(int)*n); Spid=malloc(sizeof(int)*n);
    int *fill=calloc(L,sizeof(int));
    for(long i=0;i<n;i++){ int g=gs[i]; int pos=sstart[g]+fill[g]++; Slast[pos]=ls[i]; Spid[pos]=(int)i; }
    free(fill);

    ng=malloc(sizeof(int)*L*NG);
    nl=malloc(sizeof(uint16_t)*L*NG);
    int d[5];
    for(int k=0;k<L;k++){
        digits5(k,d);
        int idx=0;
        for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)
        for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
            int c[5]={(d[0]+i0+6)%7,(d[1]+i1+6)%7,(d[2]+i2+6)%7,(d[3]+i3+6)%7,(d[4]+i4+6)%7};
            int v=0; for(int t=0;t<5;t++) v=v*7+c[t];
            ng[(size_t)k*NG+idx]=v;
            nl[(size_t)k*NG+idx]=(uint16_t)v;
            idx++;
        }
    }
    fprintf(stderr,"tables ready %.1fs\n",now());

    cnt=malloc(L); owner=malloc(sizeof(int32_t)*L);
    cand=malloc(sizeof(int)*64*(size_t)n); ccnt=calloc(n,sizeof(int));
    if(!cnt||!owner||!cand||!ccnt){fprintf(stderr,"oom\n");return 1;}

    double t0=now();
    long totalOne=0;
    for(int g2=0;g2<L;g2++){
        memset(cnt,0,L);
        memset(owner,0xFF,sizeof(int32_t)*L);
        const int *ngp=&ng[(size_t)g2*NG];
        for(int gi=0;gi<NG;gi++){
            int g1=ngp[gi];
            for(int k=sstart[g1];k<sstart[g1+1];k++){
                int l=Slast[k]; int pid=Spid[k];
                const uint16_t *nlp=&nl[(size_t)l*NG];
                for(int li=0;li<NG;li++){
                    int lp=nlp[li];
                    if(g1==g2 && lp==l) continue;   // q = p itself is not a neighbour
                    uint8_t c=cnt[lp];
                    if(c==0){ cnt[lp]=1; owner[lp]=pid; }
                    else if(c==1){ cnt[lp]=2; owner[lp]=-1; }
                    else if(c<255) cnt[lp]=c+1;
                }
            }
        }
        for(int lp=0;lp<L;lp++){
            if(cnt[lp]!=1) continue;
            totalOne++;
            int p=owner[lp];
            if(p<0) continue;
            long q=(long)g2*L+lp;
            int m=ccnt[p];
            int *cp=&cand[(size_t)p*64];
            for(int t=0;t<m;t++){
                if(!adjacent(q, cp[t])){
                    fprintf(stderr,"EXCHANGE FOUND p=%d q1=%ld q2=%ld t=%.1fs\n",p,cp[t],q,now()-t0);
                    printf("%d %ld %ld\n",p,cp[t],q);
                    return 0;
                }
            }
            if(m<64){ cp[m]=q; ccnt[p]=m+1; }
        }
        if((g2&0x3FF)==0) fprintf(stderr,"g2=%d t=%.1fs one=%ld\n",g2,now()-t0,totalOne);
    }
    fprintf(stderr,"no exchange found. total cnt==1 vertices=%ld t=%.1fs\n",totalOne,now()-t0);
    printf("NONE\n");
    return 2;
}