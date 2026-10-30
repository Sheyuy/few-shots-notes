// p004: decide maximality of the 134753-point independent set in C7^(x)10.
// Group vertices by first 5 coordinates (7^5 = 16807 groups).  A candidate v is
// addable iff no point of S lies in N(v).  forbidden[g] is a bitset over last-5
// coordinates of the closed neighbourhood of S inside group g.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define G 16807
#define W 263
#define NB 3

static int nb[7][3];
static uint64_t *forb;
static int d5[G][5];

static int digits5(int idx, int out[5]){
    for(int i=4;i>=0;i--){ out[i]=idx%7; idx/=7; }
    return 0;
}
static int val5(const int d[5]){
    int x=0; for(int i=0;i<5;i++) x=x*7+d[i]; return x;
}

int main(int argc,char**argv){
    for(int d=0;d<7;d++){ nb[d][0]=(d+6)%7; nb[d][1]=d; nb[d][2]=(d+1)%7; }
    for(int i=0;i<G;i++) digits5(i,d5[i]);

    forb = calloc((size_t)G*W, sizeof(uint64_t));
    if(!forb){ fprintf(stderr,"oom\n"); return 1; }
    FILE *f=fopen(argv[1],"r");
    long n=0; long x;
    int d[10];
    while(fscanf(f,"%ld",&x)==1){
        n++;
        for(int i=9;i>=0;i--){ d[i]=x%7; x/=7; }
        int g=val5(d), l=val5(d+5);
        // all closed-neighbourhood last-5 keys of this point
        int a[5];
        for(int i=0;i<5;i++) a[i]=d[5+i];
        for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)
        for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
            int c[5]={nb[a[0]][i0],nb[a[1]][i1],nb[a[2]][i2],nb[a[3]][i3],nb[a[4]][i4]};
            int lp=val5(c);
            forb[(size_t)g*W + (lp>>6)] |= 1ULL<<(lp&63);
        }
    }
    fclose(f);
    fprintf(stderr,"loaded %ld points\n",n);

    uint64_t *acc = malloc(sizeof(uint64_t)*W);
    long found=0; int firstg=-1, firstl=-1;
    for(int gv=0; gv<G; gv++){
        memset(acc,0,sizeof(uint64_t)*W);
        int a[5]; for(int i=0;i<5;i++) a[i]=d5[gv][i];
        for(int i0=0;i0<3;i0++)for(int i1=0;i1<3;i1++)for(int i2=0;i2<3;i2++)
        for(int i3=0;i3<3;i3++)for(int i4=0;i4<3;i4++){
            int c[5]={nb[a[0]][i0],nb[a[1]][i1],nb[a[2]][i2],nb[a[3]][i3],nb[a[4]][i4]};
            int gp=val5(c);
            const uint64_t *src=&forb[(size_t)gp*W];
            for(int w=0;w<W;w++) acc[w]|=src[w];
        }
        // bits not set in acc are addable last-5 coordinates
        for(int w=0;w<W;w++){
            uint64_t z=~acc[w];
            if(w==W-1){ int rem=G&63; uint64_t valid=(rem? ((1ULL<<rem)-1) : ~0ULL); z &= valid; }
            if(z){
                int lv=(w<<6)+__builtin_ctzll(z);
                if(lv<G){
                    found++;
                    if(firstg<0){ firstg=gv; firstl=lv; }
                }
            }
        }
    }
    fprintf(stderr,"addable vertices: %ld\n",found);
    if(firstg>=0){
        printf("%d %d\n",firstg,firstl); // group digits and last digits, base 7
        printf("first addable: first5 digits ");
        for(int i=0;i<5;i++) printf("%d",d5[firstg][i]);
        printf(" last5 digits ");
        int t=firstl; int dd[5]; for(int i=4;i>=0;i--){ dd[i]=t%7; t/=7; }
        for(int i=0;i<5;i++) printf("%d",dd[i]);
        printf("\n");
    } else printf("MAXIMAL\n");
    return 0;
}