// p006: simulated annealing on random single flips, with reheat from best.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define N 100
static int s[N], C[N], bs[N];
static long E, bestE;
static uint64_t rs=0x452821E638D01377ULL;
static inline uint64_t rnd64(void){ rs^=rs<<13; rs^=rs>>7; rs^=rs<<17; return rs; }
static inline int rndint(int n){ return (int)(rnd64()%(uint64_t)n); }
static inline double rnd(void){ return (double)(rnd64()>>11)*(1.0/9007199254740992.0); }
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }

static void compute(void){
    for(int k=1;k<N;k++){ int c=0; for(int i=0;i+k<N;i++) c+=s[i]*s[i+k]; C[k]=c; }
    long e=0; for(int k=1;k<N;k++) e+=(long)C[k]*C[k]; E=e;
}
static inline long delta(int i){
    long d=0; int si=s[i];
    for(int k=1;k<N;k++){
        int a=(i+k<N)?s[i+k]:0, b=(i-k>=0)?s[i-k]:0;
        int dc=-2*si*(a+b);
        d += 2L*C[k]*dc + (long)dc*dc;
    }
    return d;
}
static inline void flip(int i){
    int si=s[i];
    for(int k=1;k<N;k++){
        int a=(i+k<N)?s[i+k]:0, b=(i-k>=0)?s[i-k]:0;
        C[k]+=-2*si*(a+b);
    }
    s[i]=-si;
    long e=0; for(int k=1;k<N;k++) e+=(long)C[k]*C[k]; E=e;
}
static void save(void){ memcpy(bs,s,sizeof(s)); bestE=E; }
static void out(void){
    if(E<=574){
        FILE*f=fopen("best_seq_sa.txt","w");
        fprintf(f,"%ld\n",E); for(int i=0;i<N;i++) fputc(s[i]>0?'+':'-',f); fputc('\n',f); fclose(f);
        printf("FOUND %ld\n",E); exit(0);
    }
}
int main(int argc,char**argv){
    double budget=argc>1?atof(argv[1]):60.0;
    double Tmax=argc>2?atof(argv[2]):300.0;
    rs^=(uint64_t)time(NULL)*0x9E3779B97F4A7C15ULL;
    double t0=now();
    long iters=0;
    for(int i=0;i<N;i++) s[i]=(rnd64()&1)?1:-1;
    compute(); save(); out();
    while(now()-t0<budget){
        double phase=2.0; // seconds per anneal cycle
        double start=now();
        double T=Tmax;
        while(now()-start<phase && now()-t0<budget){
            int i=rndint(N);
            long d=delta(i);
            if(d<=0 || rnd()<exp(-(double)d/T)){
                flip(i);
                if(E<bestE){ save(); out(); }
            }
            iters++;
            double fr=(now()-start)/phase;
            T=Tmax*pow(0.02/Tmax,fr);
        }
        // reheat from best with a kick
        memcpy(s,bs,sizeof(s)); compute();
        for(int k=0;k<2;k++) flip(rndint(N));
        if(E<bestE){ save(); out(); }
    }
    memcpy(s,bs,sizeof(s)); compute();
    FILE*f=fopen("best_seq_sa.txt","w");
    fprintf(f,"%ld\n",E); for(int i=0;i<N;i++) fputc(s[i]>0?'+':'-',f); fputc('\n',f); fclose(f);
    fprintf(stderr,"done bestE=%ld iters=%ld\n",bestE,iters);
    return 2;
}