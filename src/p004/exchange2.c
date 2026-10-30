// p004: search for a 2-for-3 exchange: remove two points, add three pairwise
// non-adjacent points whose only neighbours in S are those two points.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define L 16807
#define NG 243
#define HS 22              // hash table 2^22 entries

static int *ng;
static uint16_t *nl;
static int *sstart, *Slast, *Spid;
static uint8_t *cnt;
static int32_t *o1, *o2;

typedef struct { uint64_t key; int n; long q[5]; } Entry;
static Entry *H;

static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
static void digits5(int idx,int d[5]){ for(int i=4;i>=0;i--){ d[i]=idx%7; idx/=7; } }
static int adjacent(long a,long b){
    for(int k=0;k<10;k++){
        int x=a%7,y=b%7,d=x-y; if(d<0)d=-d;
        if(!(d==0||d==1||d==6)) return 0;
        a/=7;b/=7;
    }
    return 1;
}
static Entry* slot(uint64_t key){
    uint64_t h=(key*0x9E3779B97F4A7C15ULL)>>(64-HS);
    for(;;){
        Entry*e=&H[h];
        if(e->key==0 || e->key==key) return e;
        h=(h+1)&((1ULL<<HS)-1);
    }
}

int main(int argc,char**argv){
    FILE *f=fopen(argv[1],"r");
    long n=0,x;
    long *pts=malloc(sizeof(long)*200000);
    int *gs=malloc(sizeof(int)*200000), *ls=malloc(sizeof(int)*200000);
    while(fscanf(f,"%ld",&x)==1){ pts[n]=x; gs[n]=x/L; ls[n]=x%L; n++; }
    fclose(f);
    fprintf(stderr,"loaded %ld points\n",n);

    sstart=calloc(L+1,sizeof(int));
    for(long i=0;i<n;i++) sstart[gs[i]+1]++;
    for(int g=0;g<L;g++) sstart[g+1]+=sstart[g];
    Slast=malloc(sizeof(int)*n); Spid=malloc(sizeof(int)*n);
    int *fill=calloc(L,sizeof(int));
    for(long i=0;i<n;i++){ int g=gs[i]; int pos=sstart[g]+fill[g]++; Slast[pos]=ls[i]; Spid[pos]=(int)i; }
    free(fill);

    ng=malloc(sizeof(int)*L*NG); nl=malloc(sizeof(uint16_t)*L*NG);
    int d[5];
    for(int k=0;k<L;k++){
        digits5(k,d); int idx=0;
        for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)
        for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
            int c[5]={(d[0]+i0+6)%7,(d[1]+i1+6)%7,(d[2]+i2+6)%7,(d[3]+i3+6)%7,(d[4]+i4+6)%7};
            int v=0; for(int t=0;t<5;t++) v=v*7+c[t];
            ng[(size_t)k*NG+idx]=v; nl[(size_t)k*NG+idx]=(uint16_t)v; idx++;
        }
    }
    cnt=malloc(L); o1=malloc(sizeof(int32_t)*L); o2=malloc(sizeof(int32_t)*L);
    H=calloc((size_t)1<<HS,sizeof(Entry));
    if(!H){fprintf(stderr,"oom\n");return 1;}
    fprintf(stderr,"tables ready %.1fs\n",now());

    double t0=now();
    long c2=0;
    for(int g2=0;g2<L;g2++){
        memset(cnt,0,L);
        memset(o1,0xFF,sizeof(int32_t)*L);
        memset(o2,0xFF,sizeof(int32_t)*L);
        const int *ngp=&ng[(size_t)g2*NG];
        for(int gi=0;gi<NG;gi++){
            int g1=ngp[gi];
            for(int k=sstart[g1];k<sstart[g1+1];k++){
                int l=Slast[k], pid=Spid[k];
                const uint16_t *nlp=&nl[(size_t)l*NG];
                for(int li=0;li<NG;li++){
                    int lp=nlp[li];
                    if(g1==g2 && lp==l) continue;
                    uint8_t c=cnt[lp];
                    if(c==0){ cnt[lp]=1; o1[lp]=pid; }
                    else if(c==1){ cnt[lp]=2; o2[lp]=pid; }
                    else if(c<255) cnt[lp]=c+1;
                }
            }
        }
        for(int lp=0;lp<L;lp++){
            if(cnt[lp]!=2) continue;
            int a=o1[lp], b=o2[lp];
            if(a<0||b<0||a==b) continue;
            c2++;
            long q=(long)g2*L+lp;
            uint64_t key=(uint64_t)(a<b?a:b)*200003ULL + (uint64_t)(a<b?b:a);
            Entry*e=slot(key+1);
            if(e->key==0){ e->key=key+1; e->q[0]=q; e->n=1; continue; }
            int ok=1;
            for(int t=0;t<e->n;t++) if(adjacent(q,e->q[t])){ ok=0; break; }
            if(ok && e->n>=2){
                fprintf(stderr,"EXCHANGE2 FOUND a=%d b=%d q1=%ld q2=%ld q3=%ld t=%.1fs\n",
                        a<b?a:b, a<b?b:a, e->q[0], e->q[1], q, now()-t0);
                printf("%d %d %ld %ld %ld\n", a<b?a:b, a<b?b:a, e->q[0], e->q[1], q);
                return 0;
            }
            if(ok && e->n<5){ e->q[e->n++]=q; }
        }
        if((g2&0x3FF)==0) fprintf(stderr,"g2=%d t=%.1fs cnt2=%ld\n",g2,now()-t0,c2);
    }
    fprintf(stderr,"no 2-for-3 exchange. cnt2=%ld t=%.1fs\n",c2,now()-t0);
    printf("NONE\n");
    return 2;
}