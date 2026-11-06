#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define J 30
#define M 20
#define NOP (J*M)
static int route[J][M], dur[J][M];
static int posjob[J][M];        /* posjob[j][m] = k */
static int order[M][J];         /* order[m][p] = job */
static int bestorder[M][J];
static int jobpred[NOP], jobsucc[NOP];

static uint64_t rng=0x853c49e6748fea9bULL;
static inline uint64_t rnd(void){uint64_t x=rng;x^=x<<13;x^=x>>7;x^=x<<17;rng=x;return x;}
static inline int rndint(int n){return (int)(rnd()%(uint64_t)n);}

static int machpred[NOP], machsucc[NOP];
static int indeg[NOP], head[NOP], tail[NOP];
static int queue[NOP];

/* evaluate given orders; returns makespan, -1 if cycle. head/tail filled if non-null */
static int evaluate(int ord[M][J], int *headp, int *tailp){
    for(int m=0;m<M;m++){
        for(int p=0;p<J;p++){
            int j=ord[m][p]; int op=j*M+posjob[j][m];
            if(p>0){int jp=ord[m][p-1]; int opq=jp*M+posjob[jp][m]; machpred[op]=opq; machsucc[opq]=op;}
            else machpred[op]=-1;
            if(p<J-1){}
            else machsucc[op]=-1;
        }
    }
    for(int op=0;op<NOP;op++){
        int j=op/M,k=op%M;
        jobpred[op]=(k>0)?op-1:-1;
        jobsucc[op]=(k<M-1)?op+1:-1;
        indeg[op]=((jobpred[op]>=0)?1:0)+((machpred[op]>=0)?1:0);
        head[op]=0;
    }
    int qh=0,qt=0;
    for(int op=0;op<NOP;op++) if(indeg[op]==0) queue[qt++]=op;
    int cnt=0;
    while(qh<qt){
        int op=queue[qh++]; cnt++;
        int comp=head[op]+dur[op/M][op%M];
        int s=jobsucc[op]; if(s>=0){ if(comp>head[s])head[s]=comp; if(--indeg[s]==0)queue[qt++]=s; }
        s=machsucc[op]; if(s>=0){ if(comp>head[s])head[s]=comp; if(--indeg[s]==0)queue[qt++]=s; }
    }
    if(cnt<NOP) return -1;
    int mk=0;
    for(int op=0;op<NOP;op++){int c=head[op]+dur[op/M][op%M]; if(c>mk)mk=c;}
    if(tailp){
        /* reverse topological order: queue[] already holds a topo order */
        for(int i=0;i<NOP;i++){
            int op=queue[NOP-1-i];
            int t=dur[op/M][op%M];
            int s=jobsucc[op]; if(s>=0){int v=t+tailp[s]; if(v>t)t=v;}
            s=machsucc[op]; if(s>=0){int v=t+tailp[s]; if(v>t)t=v;}
            tailp[op]=t;
        }
    }
    return mk;
}

static int tabu[NOP][NOP];  /* too big? 600*600 ints = 1.4MB ok */
static int cur[M][J];

static void copy_orders(int dst[M][J], int src[M][J]){memcpy(dst,src,sizeof(int)*M*J);}

int main(int argc,char**argv){
    FILE*f=fopen("instance.txt","r"); int nj,nm; if(fscanf(f,"%d %d",&nj,&nm)!=2)return 1;
    for(int j=0;j<J;j++)for(int k=0;k<M;k++){ if(fscanf(f,"%d %d",&route[j][k],&dur[j][k])!=2)return 1; posjob[j][route[j][k]]=k; }
    fclose(f);
    f=fopen("orders.txt","r");
    for(int m=0;m<M;m++)for(int p=0;p<J;p++) if(fscanf(f,"%d",&cur[m][p])!=1) return 1;
    fclose(f);
    int mk=evaluate(cur,NULL,NULL);
    printf("baseline makespan %d\n",mk);
    int best=mk; copy_orders(bestorder,cur);
    if(argc>1) rng ^= strtoull(argv[1],0,10)*0x9e3779b97f4a7c15ULL+1;
    long deadline = argc>2?atol(argv[2]):600;

    static int tail[NOP], head[NOP];
    long t0=time(NULL);
    long it=0, since=0;
    int tenure=10;
    while(1){
        it++;
        if((it&1023)==0 && time(NULL)-t0>deadline) break;
        int mkcur=evaluate(cur,head,tail);
        if(mkcur<0){ /* cycle: restore best and kick */
            copy_orders(cur,bestorder); evaluate(cur,head,tail); mkcur=best;
        }
        /* collect critical adjacent machine pairs */
        int ca[2000],cb[2000],nc=0;
        int mkall=mkcur;
        for(int m=0;m<M;m++){
            for(int p=0;p+1<J;p++){
                int j1=cur[m][p], j2=cur[m][p+1];
                int o1=j1*M+posjob[j1][m], o2=j2*M+posjob[j2][m];
                if(head[o1]+dur[j1][posjob[j1][m]]==head[o2] &&
                   head[o1]+tail[o1]==mkall && head[o2]+tail[o2]==mkall){
                    if(nc<2000){ca[nc]=m;cb[nc]=p;nc++;}
                }
            }
        }
        if(nc==0){
            /* fallback: any adjacent pair on a machine */
            for(int m=0;m<M;m++) for(int p=0;p+1<J;p++){ if(nc<2000){ca[nc]=m;cb[nc]=p;nc++;} }
        }
        int bestmove=-1,bestval=1000000000;
        /* evaluate moves */
        for(int i=0;i<nc;i++){
            int m=ca[i],p=cb[i];
            int j1=cur[m][p], j2=cur[m][p+1];
            int o1=j1*M+posjob[j1][m], o2=j2*M+posjob[j2][m];
            int rev = tabu[o2][o1] > it;
            cur[m][p]=j2; cur[m][p+1]=j1;
            int val=evaluate(cur,NULL,NULL);
            cur[m][p]=j1; cur[m][p+1]=j2;
            if(val<0) continue;
            if(rev && val>=best) continue;
            if(val<bestval || (val==bestval && rndint(2))){
                bestval=val; bestmove=i;
            }
        }
        if(bestmove<0){
            /* random kick: swap a random adjacent pair */
            int m=rndint(M),p=rndint(J-1);
            int j1=cur[m][p];cur[m][p]=cur[m][p+1];cur[m][p+1]=j1;
            if(evaluate(cur,NULL,NULL)<0){cur[m][p+1]=cur[m][p];cur[m][p]=j1;}
            continue;
        }
        int m=ca[bestmove],p=cb[bestmove];
        int j1=cur[m][p], j2=cur[m][p+1];
        int o1=j1*M+posjob[j1][m], o2=j2*M+posjob[j2][m];
        cur[m][p]=j2; cur[m][p+1]=j1;
        tabu[o1][o2]=it+tenure+rndint(5);
        if(bestval<best){
            best=bestval; copy_orders(bestorder,cur); since=0;
            printf("iter %ld: new best %d (t=%lds)\n",it,best,(long)(time(NULL)-t0)); fflush(stdout);
            FILE*o=fopen("best_orders.txt","w");
            for(int mm=0;mm<M;mm++){for(int pp=0;pp<J;pp++)fprintf(o,"%d ",bestorder[mm][pp]);fprintf(o,"\n");}
            fclose(o);
            if(best<=2005){printf("TARGET REACHED %d\n",best);return 0;}
        } else since++;
        if(since>20000){
            /* diversify: restart from best + random adjacent swaps */
            copy_orders(cur,bestorder);
            for(int t=0;t<30;t++){
                int mm=rndint(M),pp=rndint(J-1);
                int a=cur[mm][pp];cur[mm][pp]=cur[mm][pp+1];cur[mm][pp+1]=a;
                if(evaluate(cur,NULL,NULL)<0){cur[mm][pp+1]=cur[mm][pp];cur[mm][pp]=a;}
            }
            memset(tabu,0,sizeof(tabu));
            since=0;
        }
    }
    printf("done best=%d iters=%ld\n",best,it);
    return 0;
}
