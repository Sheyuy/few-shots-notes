// p001: is there room for one more vector on the sphere (angle >= 60 deg from 604 vectors)?
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define D 11
#define M 604
static double V[M][D];

static unsigned long long rs=123456789;
static inline double rnd01(void){ rs^=rs<<13; rs^=rs>>7; rs^=rs<<17; return (double)(rs>>11)*(1.0/9007199254740992.0); }

int main(int argc,char**argv){
    const char *path=argv[1];
    FILE *f=fopen(path,"r");
    // file: 604 lines of 22 integers (p,q pairs)
    int m=M;
    for(int i=0;i<m;i++){
        for(int k=0;k<D;k++){
            long p,q; if(fscanf(f,"%ld %ld",&p,&q)!=2){fprintf(stderr,"parse fail %d %d\n",i,k);return 1;}
            V[i][k]=p+q*sqrt(2.0);
        }
    }
    fclose(f);
    double maxlen=0; for(int i=0;i<m;i++){double s=0;for(int k=0;k<D;k++)s+=V[i][k]*V[i][k]; if(fabs(sqrt(s)-6)>1e-9) maxlen=fmax(maxlen,fabs(sqrt(s)-6));}
    fprintf(stderr,"max length deviation %.3g\n",maxlen);

    int trials=50, iters=4000;
    double bestviol=1e9;
    for(int t=0;t<trials;t++){
        double x[D], n2=0;
        for(int k=0;k<D;k++){ x[k]=2*rnd01()-1; n2+=x[k]*x[k]; }
        double n=sqrt(n2); for(int k=0;k<D;k++) x[k]*=6.0/n;
        double mv=0;
        for(int it=0;it<iters;it++){
            mv=-1e9; int mi=0;
            for(int j=0;j<m;j++){
                double d=-18.0; for(int k=0;k<D;k++) d+=x[k]*V[j][k];
                if(d>mv){ mv=d; mi=j; }
            }
            if(mv<1e-8) break;
            for(int k=0;k<D;k++) x[k]-=(mv/36.0)*V[mi][k];
            n2=0; for(int k=0;k<D;k++) n2+=x[k]*x[k];
            n=sqrt(n2); for(int k=0;k<D;k++) x[k]*=6.0/n;
        }
        // final check
        double worst=-1e9;
        for(int j=0;j<m;j++){ double d=-18.0; for(int k=0;k<D;k++) d+=x[k]*V[j][k]; if(d>worst) worst=d; }
        if(worst<bestviol){ bestviol=worst; fprintf(stderr,"trial %d worst %.6f\n",t,worst); }
        if(worst<1e-6){
            printf("FEASIBLE %.9f\n",worst);
            for(int k=0;k<D;k++) printf("%.9f ",x[k]);
            printf("\n");
            return 0;
        }
    }
    printf("no feasible found, best violation %.6f\n",bestviol);
    return 2;
}