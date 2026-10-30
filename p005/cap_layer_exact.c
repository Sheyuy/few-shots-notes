#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M7 2187
#define M8 6561
#define N0 512
#define MAXR 2187
#define MAXT 2048
#define NW 32

static short third7[M7][M7];
static int pow3[10];
static int ntrip, nw;
static int tA[MAXT], tB[MAXT], tC[MAXT];
static unsigned long long vmask[M7][NW];
static unsigned long long fullmask[NW];

static int dfs(unsigned long long cov[NW], int budget, int *chosen, int depth) {
    int all = 1;
    for (int w = 0; w < nw; w++) if ((cov[w] & fullmask[w]) != fullmask[w]) { all = 0; break; }
    if (all) return depth;
    if (budget == 0) return -1;
    int t = -1;
    for (int w = 0; w < nw; w++) { unsigned long long u = (~cov[w]) & fullmask[w]; if (u) { t = w*64 + __builtin_ctzll(u); break; } }
    if (t < 0 || t >= ntrip) return -1;
    int vs[3] = { tA[t], tB[t], tC[t] };
    for (int q = 0; q < 3; q++) {
        int v = vs[q];
        unsigned long long nc[NW];
        for (int w = 0; w < nw; w++) nc[w] = cov[w] | vmask[v][w];
        chosen[depth] = v;
        int r = dfs(nc, budget-1, chosen, depth+1);
        if (r >= 0) return r;
    }
    return -1;
}

int main(void) {
    pow3[0]=1; for(int k=1;k<10;k++) pow3[k]=pow3[k-1]*3;
    for (int a=0;a<M7;a++) for(int b=0;b<M7;b++){
        int x=a,y=b,z=0,pw=1;
        for(int k=0;k<7;k++){int s=((x%3)+(y%3))%3;int c=(3-s)%3;z+=c*pw;pw*=3;x/=3;y/=3;}
        third7[a][b]=(short)z;
    }
    FILE *f=fopen("baseline_pts.txt","r"); if(!f){fprintf(stderr,"no baseline\n");return 1;}
    int pts[N0];
    for(int t=0;t<N0;t++){
        int v[8]; for(int k=0;k<8;k++) if(fscanf(f,"%d",&v[k])!=1){fprintf(stderr,"parse\n");return 1;}
        int c=0; for(int k=0;k<8;k++) c+=v[k]*pow3[k];
        pts[t]=c;
    }
    fclose(f);

    static char inl[3][M7]; static int lay[3][M7], lsz[3];
    for (int c=0;c<8;c++){
        memset(inl,0,sizeof(inl)); lsz[0]=lsz[1]=lsz[2]=0;
        for(int t=0;t<N0;t++){
            int d=(pts[t]/pow3[c])%3;
            /* proj: remove digit c, little-endian */
            int z=0,m=0;
            for(int k=0;k<8;k++){ if(k==c) continue; int dd=(pts[t]/pow3[k])%3; z+=dd*pow3[m]; m++; }
            if(!inl[d][z]){ inl[d][z]=1; lay[d][lsz[d]++]=z; }
        }
        printf("coord %d: sizes %d/%d/%d\n", c, lsz[0],lsz[1],lsz[2]);
        for(int i=0;i<3;i++){
            int j=(i+1)%3, k=(i+2)%3;
            static char forb[M7]; memset(forb,0,M7);
            for(int u=0;u<lsz[j];u++) for(int v=0;v<lsz[k];v++) forb[third7[lay[j][u]][lay[k][v]]]=1;
            int R[MAXR], Rn=0; static int pos[M7];
            for(int x=0;x<M7;x++) pos[x]=-1;
            for(int x=0;x<M7;x++) if(!forb[x]){ pos[x]=Rn; R[Rn++]=x; }
            if (Rn <= lsz[i]) { printf("   layer %d: |R|=%d <= size %d  -> no improvement possible\n", i, Rn, lsz[i]); continue; }
            if (Rn > 300) { printf("   layer %d: |R|=%d (too big for exact; size %d)\n", i, Rn, lsz[i]); continue; }
            /* enumerate triples */
            ntrip=0;
            for(int u=0;u<Rn;u++) for(int v=u+1;v<Rn;v++){
                int r=third7[R[u]][R[v]]; int w=pos[r];
                if(w>v && ntrip<MAXT){ tA[ntrip]=u;tB[ntrip]=v;tC[ntrip]=w;ntrip++; }
            }
            nw=(ntrip+63)/64; if(nw==0)nw=1;
            if(nw>NW){ printf("   layer %d: too many triples %d\n", i, ntrip); continue; }
            for(int w=0;w<nw;w++) fullmask[w]=~0ULL;
            if(ntrip%64) fullmask[nw-1]=(1ULL<<(ntrip%64))-1;
            memset(vmask,0,sizeof(vmask));
            for(int t=0;t<ntrip;t++){ int vs[3]={tA[t],tB[t],tC[t]}; for(int q=0;q<3;q++) vmask[vs[q]][t/64]|=(1ULL<<(t%64)); }
            int minhs=-1, chosen[MAXR];
            for(int b=0;b<=20;b++){ unsigned long long cov[NW]; memset(cov,0,sizeof(cov)); int r=dfs(cov,b,chosen,0); if(r>=0){minhs=r;break;} }
            if(minhs<0){ printf("   layer %d: |R|=%d triples=%d minHS>20 (no exact)\n", i, Rn, ntrip); }
            else { int mc=Rn-minhs; printf("   layer %d: |R|=%d triples=%d minHS=%d exact_maxcap=%d  size=%d %s\n",
                   i, Rn, ntrip, minhs, mc, lsz[i], mc>lsz[i]?"<== IMPROVE":"(no gain)"); }
        }
    }
    return 0;
}
