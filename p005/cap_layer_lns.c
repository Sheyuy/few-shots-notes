#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M7 2187
#define N0 512
#define MAXN 2400

static short t7[M7][M7];
static int pow3[10];
static unsigned long st = 88172645463325252UL;
static unsigned int xr(void){st^=st<<13;st^=st>>7;st^=st<<17;return (unsigned int)(st>>11);}

/* strong max cap inside candidate set R: greedy restarts + ruin&recreate ILS */
static int strong_maxcap(const int *R, int Rn, int greedyRestarts, int ilsIters,
                         int *bestOut, int *work, char *mem, int *ord) {
    int best = 0;
    for (int r = 0; r < greedyRestarts; r++) {
        memcpy(ord, R, Rn*sizeof(int));
        for (int i=Rn-1;i>0;i--){int j=xr()%(i+1);int t=ord[i];ord[i]=ord[j];ord[j]=t;}
        memset(mem,0,M7);
        int nc=0;
        for(int q=0;q<Rn;q++){int p=ord[q],ok=1;for(int w=0;w<nc;w++){if(mem[t7[p][work[w]]]){ok=0;break;}}if(ok){mem[p]=1;work[nc++]=p;}}
        if(nc>best){best=nc;memcpy(bestOut,work,nc*sizeof(int));}
    }
    /* ILS on current best */
    memcpy(work,bestOut,best*sizeof(int));
    int nc=best;
    memset(mem,0,M7); for(int i=0;i<nc;i++) mem[work[i]]=1;
    for(int it=0; it<ilsIters; it++){
        int r = 2 + (int)(xr()%6);   /* remove 2..7 */
        if(r>nc) r=nc;
        /* remove r random */
        for(int q=0;q<r;q++){
            int idx=xr()%(unsigned)nc;
            int a=work[idx];
            /* remove a: swap with last */
            work[idx]=work[nc-1]; nc--; mem[a]=0;
        }
        /* refill: repeatedly add addable in random order */
        int added=0;
        for(int pass=0; pass<3; pass++){
            /* collect addable */
            int na=0;
            for(int q=0;q<Rn;q++){ int p=R[q]; if(!mem[p]){ int ok=1; for(int w=0;w<nc;w++){ if(mem[t7[p][work[w]]]){ok=0;break;} } if(ok) ord[na++]=p; } }
            if(na==0) break;
            for(int i=na-1;i>0;i--){int j=xr()%(i+1);int t=ord[i];ord[i]=ord[j];ord[j]=t;}
            int any=0;
            for(int i=0;i<na;i++){ int p=ord[i]; if(mem[p])continue; int ok=1; for(int w=0;w<nc;w++){ if(mem[t7[p][work[w]]]){ok=0;break;} } if(ok){ mem[p]=1; work[nc++]=p; any=1; added++; } }
            if(!any) break;
        }
        if(nc>best){ best=nc; memcpy(bestOut,work,nc*sizeof(int)); }
        /* if we dropped too far, restart from best */
        if(nc < best - 12){
            memcpy(work,bestOut,best*sizeof(int)); nc=best;
            memset(mem,0,M7); for(int i=0;i<nc;i++) mem[work[i]]=1;
        }
    }
    return best;
}

int main(int argc, char **argv) {
    int gR = (argc>1)? atoi(argv[1]) : 200;
    int ils= (argc>2)? atoi(argv[2]) : 60000;
    pow3[0]=1; for(int k=1;k<10;k++) pow3[k]=pow3[k-1]*3;
    for(int a=0;a<M7;a++)for(int b=0;b<M7;b++){
        int x=a,y=b,z=0,pw=1;
        for(int k=0;k<7;k++){int s=((x%3)+(y%3))%3;int c=(3-s)%3;z+=c*pw;pw*=3;x/=3;y/=3;}
        t7[a][b]=(short)z;
    }
    FILE *f=fopen("baseline_pts.txt","r"); if(!f){fprintf(stderr,"no baseline\n");return 1;}
    int pts[N0];
    for(int i=0;i<N0;i++){int v[8];for(int k=0;k<8;k++)if(fscanf(f,"%d",&v[k])!=1){fprintf(stderr,"parse\n");return 1;}int c=0;for(int k=0;k<8;k++)c+=v[k]*pow3[k];pts[i]=c;}
    fclose(f);

    static char inl[3][M7]; static int lay[3][M7], lsz[3];
    int R[M7], bestOut[MAXN], work[MAXN], ord[M7]; char mem[M7];
    int anyGain=0;
    for(int c=0;c<8;c++){
        memset(inl,0,sizeof(inl)); lsz[0]=lsz[1]=lsz[2]=0;
        for(int i=0;i<N0;i++){
            int d=(pts[i]/pow3[c])%3; int z=0,m=0;
            for(int k=0;k<8;k++){ if(k==c)continue; int dd=(pts[i]/pow3[k])%3; z+=dd*pow3[m]; m++; }
            if(!inl[d][z]){ inl[d][z]=1; lay[d][lsz[d]++]=z; }
        }
        for(int i=0;i<3;i++){
            int j=(i+1)%3,k=(i+2)%3;
            static char forb[M7]; memset(forb,0,M7);
            for(int u=0;u<lsz[j];u++)for(int v=0;v<lsz[k];v++) forb[t7[lay[j][u]][lay[k][v]]]=1;
            int Rn=0; for(int x=0;x<M7;x++) if(!forb[x]) R[Rn++]=x;
            int m=strong_maxcap(R,Rn,gR,ils,bestOut,work,mem,ord);
            printf("c=%d L%d: size=%d |R|=%d strongMaxcap=%d %s\n",c,i,lsz[i],Rn,m,m>lsz[i]?"<== IMPROVE!":"");
            fflush(stdout);
            if(m>lsz[i]){ anyGain=1; }
        }
    }
    if(!anyGain) printf("NO single-layer improvement found (strong LNS).\n");
    return 0;
}
