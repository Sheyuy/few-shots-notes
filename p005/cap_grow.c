#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define D7 7
#define M7 2187
#define NFULL 512
#define MAXB 2048

static short third7[M7][M7];
static int enc7(const int *v){int x=0;for(int k=D7-1;k>=0;k--)x=x*3+v[k];return x;}
static void dec7(int x,int *v){for(int k=0;k<D7;k++){v[k]=x%3;x/=3;}}

static unsigned long st;
static unsigned int xr(void){st^=st<<13;st^=st>>7;st^=st<<17;return (unsigned int)(st>>11);}

static int buildR(const int *B,int Bn,int *R){
    static char forb[M7];
    memset(forb,0,sizeof(forb));
    for(int i=0;i<Bn;i++) forb[B[i]]=1;
    for(int i=0;i<Bn;i++) for(int j=i+1;j<Bn;j++) forb[third7[B[i]][B[j]]]=1;
    int n=0; for(int x=0;x<M7;x++) if(!forb[x]) R[n++]=x;
    return n;
}
static int maxcap(const int *R,int Rn,int restarts,int *bestC,int *ord,char *mem,int *cur){
    int bestn=0;
    for(int r=0;r<restarts;r++){
        memcpy(ord,R,Rn*sizeof(int));
        for(int i=Rn-1;i>0;i--){int j=xr()%(i+1);int t=ord[i];ord[i]=ord[j];ord[j]=t;}
        memset(mem,0,M7);
        int ncur=0;
        for(int i=0;i<Rn;i++){
            int p=ord[i],ok=1;
            for(int j=0;j<ncur;j++){ if(mem[third7[p][cur[j]]]){ok=0;break;} }
            if(ok){mem[p]=1;cur[ncur++]=p;}
        }
        if(ncur>bestn){bestn=ncur;if(bestC)memcpy(bestC,cur,ncur*sizeof(int));}
    }
    return bestn;
}
/* p addable to cap B iff no b in B has third(p,b) in B */

int main(int argc,char**argv){
    int restarts=(argc>1)?atoi(argv[1]):12;
    int trials=(argc>2)?atoi(argv[2]):200;
    st=(argc>3)?(unsigned long)atoll(argv[3]):88172645463325252UL; if(!st)st=1;

    for(int a=0;a<M7;a++)for(int b=0;b<M7;b++){
        int x=a,y=b,z=0,pw=1;
        for(int k=0;k<D7;k++){int s=((x%3)+(y%3))%3;int c=(3-s)%3;z+=c*pw;pw*=3;x/=3;y/=3;}
        third7[a][b]=(short)z;
    }
    FILE*f=fopen("baseline_pts.txt","r"); if(!f){fprintf(stderr,"no baseline\n");return 1;}
    int base[MAXB],nbase=0; static char seen[M7];
    for(int t=0;t<NFULL;t++){
        int v[8]; for(int k=0;k<8;k++) if(fscanf(f,"%d",&v[k])!=1){fprintf(stderr,"parse\n");return 1;}
        if(v[0]!=0){ int c=enc7(&v[1]); if(!seen[c]){seen[c]=1;base[nbase++]=c;} }
    }
    fclose(f);
    printf("baseline |B|=%d\n",nbase);

    int *R=malloc(M7*sizeof(int));
    int *bestC=malloc(M7*sizeof(int));
    int *ord=malloc(M7*sizeof(int));
    int *cur=malloc(M7*sizeof(int));
    char *mem=malloc(M7);

    /* baseline score */
    int Rn=buildR(base,nbase,R);
    int mc=maxcap(R,Rn,restarts,NULL,ord,mem,cur);
    printf("baseline: |R|=%d maxcap=%d score=%d\n",Rn,mc,2*nbase+mc);

    int globalBest=0, gBn=0, gmc=0; int gB[MAXB], gC[MAXB];

    for(int tr=0;tr<trials;tr++){
        int B[MAXB]; int Bn=0;
        memcpy(B,base,nbase*sizeof(int)); Bn=nbase;
        static char inB[M7]; memset(inB,0,M7); for(int i=0;i<Bn;i++) inB[B[i]]=1;
        int localBest=-1,lBn=0,lmc=0; int lB[MAXB],lC[2048];
        for(int step=0; step<120; step++){
            /* evaluate current B */
            int Rn2=buildR(B,Bn,R);
            int m=maxcap(R,Rn2,restarts,bestC,ord,mem,cur);
            int sc=2*Bn+m;
            if(sc>localBest){ localBest=sc; lBn=Bn; lmc=m; memcpy(lB,B,Bn*sizeof(int)); memcpy(lC,bestC,m*sizeof(int)); }
            if(sc>globalBest){ globalBest=sc; gBn=Bn; gmc=m; memcpy(gB,B,Bn*sizeof(int)); memcpy(gC,bestC,m*sizeof(int));
                printf("trial %d step %d: NEW GLOBAL score=%d |B|=%d maxcap=%d |R|=%d\n",tr,step,sc,Bn,m,Rn2); fflush(stdout); }
            /* pick a random addable point (scan all points of F_3^7) */
            static int cands[M7];
            int nc=0;
            for(int p=0;p<M7;p++){
                if(inB[p]) continue;
                int ok=1;
                for(int i=0;i<Bn;i++){ if(inB[third7[p][B[i]]]){ok=0;break;} }
                if(ok) cands[nc++]=p;
            }
            if(nc==0) break;
            int found=cands[xr()%(unsigned)nc];
            B[Bn++]=found; inB[found]=1;
        }
    }
    printf("FINAL global best score=%d |B|=%d maxcap=%d\n",globalBest,gBn,gmc);
    if(globalBest>=513){
        printf("*** SUCCESS score %d ***\n",globalBest);
        /* write candidate: B x {1,2} + C x {0} */
        FILE*o=fopen("candidate_pts.txt","w");
        for(int i=0;i<gBn;i++){int v[D7];dec7(gB[i],v);
            fprintf(o,"1 %d %d %d %d %d %d %d\n",v[0],v[1],v[2],v[3],v[4],v[5],v[6]);
            fprintf(o,"2 %d %d %d %d %d %d %d\n",v[0],v[1],v[2],v[3],v[4],v[5],v[6]);}
        for(int i=0;i<gmc;i++){int v[D7];dec7(gC[i],v);
            fprintf(o,"0 %d %d %d %d %d %d %d\n",v[0],v[1],v[2],v[3],v[4],v[5],v[6]);}
        fclose(o);
        printf("wrote candidate_pts.txt (%d pts)\n",2*gBn+gmc);
    }
    return 0;
}
