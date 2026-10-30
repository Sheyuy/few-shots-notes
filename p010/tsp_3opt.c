#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define N 100000
#define K 12
#define GX 100

static int X[N], Y[N];
static int tour[N], pos[N];

static inline int dst(int i, int j) {
    int dx = X[i]-X[j], dy = Y[i]-Y[j];
    int s = dx*dx + dy*dy;
    int r = (int)sqrt((double)s);
    while ((long long)r*r > s) r--;
    while ((long long)(r+1)*(r+1) <= s) r++;
    return (s > r*r + r) ? r+1 : r;
}
static inline int nxt(int a){ int p=pos[a]+1; return tour[p>=N?0:p]; }

static int cellStart[GX*GX+1], cellItems[N], cellOf[N];
static int nb[N][K], nbd[N][K], nbc[N];

/* the 7 reconnections, endpoints indexed A=0,B=1,C=2,D=3,E=4,F=5 */
static const int MATCH[7][3][2] = {
    {{0,1},{2,4},{3,5}},   /* AB CE DF  (2-opt) */
    {{0,2},{1,3},{4,5}},   /* AC BD EF  (2-opt) */
    {{0,2},{1,4},{3,5}},   /* AC BE DF  (3-opt) */
    {{0,3},{1,4},{2,5}},   /* AD BE CF  (3-opt) */
    {{0,3},{1,5},{2,4}},   /* AD BF CE  (3-opt) */
    {{0,4},{1,3},{2,5}},   /* AE BD CF  (3-opt) */
    {{0,4},{1,5},{2,3}}    /* AE BF CD  (2-opt) */
};

int main(void) {
    FILE *f = fopen("coords.txt","r");
    if(!f){fprintf(stderr,"no coords\n");return 1;}
    for(int i=0;i<N;i++) if(fscanf(f,"%d %d",&X[i],&Y[i])!=2) return 1;
    fclose(f);
    FILE *g = fopen("baseline_tour.txt","r");
    if(!g){fprintf(stderr,"no tour\n");return 1;}
    for(int i=0;i<N;i++) if(fscanf(g,"%d",&tour[i])!=1) return 1;
    fclose(g);
    for(int i=0;i<N;i++) pos[tour[i]] = i;
    long long len=0; for(int i=0;i<N;i++) len += dst(tour[i], tour[(i+1)%N]);
    printf("tour length = %lld\n", len); fflush(stdout);

    int mnx=1<<30,mny=1<<30,mxx=-(1<<30),mxy=-(1<<30);
    for(int i=0;i<N;i++){ if(X[i]<mnx)mnx=X[i]; if(X[i]>mxx)mxx=X[i]; if(Y[i]<mny)mny=Y[i]; if(Y[i]>mxy)mxy=Y[i]; }
    double cw=(double)(mxx-mnx+1)/GX, ch=(double)(mxy-mny+1)/GX;
    static int cnt[GX*GX];
    for(int i=0;i<N;i++){
        int cx=(int)((X[i]-mnx)/cw); if(cx>=GX)cx=GX-1;
        int cy=(int)((Y[i]-mny)/ch); if(cy>=GX)cy=GX-1;
        cellOf[i]=cy*GX+cx; cnt[cellOf[i]]++;
    }
    int s=0; for(int c=0;c<GX*GX;c++){ cellStart[c]=s; s+=cnt[c]; }
    cellStart[GX*GX]=s;
    static int fillc[GX*GX];
    for(int i=0;i<N;i++){ int c=cellOf[i]; cellItems[cellStart[c]+fillc[c]++]=i; }
    for(int i=0;i<N;i++){
        int cx=(int)((X[i]-mnx)/cw); if(cx>=GX)cx=GX-1;
        int cy=(int)((Y[i]-mny)/ch); if(cy>=GX)cy=GX-1;
        int found=0,r=0;
        while(found<K && r<GX){
            for(int yy=cy-r; yy<=cy+r; yy++){ if(yy<0||yy>=GX)continue;
                for(int xx=cx-r; xx<=cx+r; xx++){ if(xx<0||xx>=GX)continue;
                    if((abs(xx-cx)!=r)&&(abs(yy-cy)!=r))continue;
                    int c=yy*GX+xx;
                    for(int t=cellStart[c]; t<cellStart[c+1]; t++){ int j=cellItems[t]; if(j==i)continue;
                        if(found<K){ nb[i][found]=j; nbd[i][found]=dst(i,j); found++; } }
                }
            }
            r++;
        }
        nbc[i]=found;
        for(int a=1;a<found;a++){ int vj=nb[i][a],vd=nbd[i][a],b=a-1;
            while(b>=0&&nbd[i][b]>vd){nb[i][b+1]=nb[i][b];nbd[i][b+1]=nbd[i][b];b--;}
            nb[i][b+1]=vj;nbd[i][b+1]=vd; }
    }
    printf("kNN built\n"); fflush(stdout);

    long long cnt3=0; int best3=0; int bA=-1,bC=-1,bE=-1,bcase=-1;
    for(int ti=0; ti<N; ti++){
        int A = tour[ti];
        int pA = ti;
        int B = tour[(ti+1)%N];
        int dAB = dst(A,B);
        for(int ci=0; ci<nbc[B]; ci++){
            int C = nb[B][ci];
            int oC = pos[C]-pA; if(oC<0) oC+=N;
            if(oC<2 || oC>N-3) continue;
            int D = nxt(C);
            int dCD = dst(C,D);
            for(int ei=0; ei<nbc[D]; ei++){
                int E = nb[D][ei];
                int oE = pos[E]-pA; if(oE<0) oE+=N;
                if(oE < oC+2 || oE > N-2) continue;
                int F = nxt(E);
                int dEF = dst(E,F);
                if(F==A) continue;
                int node[6] = {A,B,C,D,E,F};
                int base = dAB + dCD + dEF;
                for(int cs=0; cs<7; cs++){
                    int sum = dst(node[MATCH[cs][0][0]], node[MATCH[cs][0][1]])
                            + dst(node[MATCH[cs][1][0]], node[MATCH[cs][1][1]])
                            + dst(node[MATCH[cs][2][0]], node[MATCH[cs][2][1]]);
                    int gain = base - sum;
                    if(gain>0){ cnt3++; if(gain>best3){ best3=gain; bA=A; bC=C; bE=E; bcase=cs; } }
                }
            }
        }
        if((ti & 8191)==0){ }
    }
    printf("3-opt improving moves: %lld   best gain %d (A=%d C=%d E=%d case=%d)\n", cnt3, best3, bA, bC, bE, bcase);
    fflush(stdout);
    return 0;
}
