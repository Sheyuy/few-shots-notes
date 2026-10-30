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
static inline int prv(int a){ int p=pos[a]-1; return tour[p<0?N-1:p]; }

static int cellStart[GX*GX+1], cellItems[N], cellOf[N];
static int nb[N][K], nbd[N][K], nbc[N];

int main(void) {
    FILE *f = fopen("coords.txt","r");
    if(!f){fprintf(stderr,"no coords\n");return 1;}
    for(int i=0;i<N;i++) if(fscanf(f,"%d %d",&X[i],&Y[i])!=2){fprintf(stderr,"parse coords\n");return 1;}
    fclose(f);
    FILE *g = fopen("baseline_tour.txt","r");
    if(!g){fprintf(stderr,"no tour\n");return 1;}
    for(int i=0;i<N;i++) if(fscanf(g,"%d",&tour[i])!=1){fprintf(stderr,"parse tour\n");return 1;}
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
    printf("grid built\n"); fflush(stdout);

    for(int i=0;i<N;i++){
        int cx=(int)((X[i]-mnx)/cw); if(cx>=GX)cx=GX-1;
        int cy=(int)((Y[i]-mny)/ch); if(cy>=GX)cy=GX-1;
        int found=0,r=0;
        while(found<K && r<GX){
            for(int yy=cy-r; yy<=cy+r; yy++){ if(yy<0||yy>=GX)continue;
                for(int xx=cx-r; xx<=cx+r; xx++){ if(xx<0||xx>=GX)continue;
                    if((abs(xx-cx)!=r)&&(abs(yy-cy)!=r))continue;
                    int c=yy*GX+xx;
                    for(int t=cellStart[c]; t<cellStart[c+1]; t++){
                        int j=cellItems[t]; if(j==i)continue;
                        if(found<K){ nb[i][found]=j; nbd[i][found]=dst(i,j); found++; }
                    }
                }
            }
            r++;
        }
        nbc[i]=found;
        for(int a=1;a<found;a++){
            int vj=nb[i][a], vd=nbd[i][a], b=a-1;
            while(b>=0 && nbd[i][b]>vd){ nb[i][b+1]=nb[i][b]; nbd[i][b+1]=nbd[i][b]; b--; }
            nb[i][b+1]=vj; nbd[i][b+1]=vd;
        }
    }
    printf("kNN built\n"); fflush(stdout);

    /* ---- 2-opt scan (each edge is (a, next(a)) for exactly one a) ---- */
    long long cnt2=0; int best2=0; int ba=-1,bc=-1;
    for(int a=0;a<N;a++){
        int bb = nxt(a);
        int dab = dst(a,bb);
        for(int t=0;t<nbc[a];t++){
            int c=nb[a][t]; int dac=nbd[a][t];
            if(dac>=dab) break;
            int d=nxt(c);
            if(c==a||d==a||c==bb||d==bb) continue;
            int gain = dab + dst(c,d) - dac - dst(bb,d);
            if(gain>0){ cnt2++; if(gain>best2){ best2=gain; ba=a; bc=c; } }
        }
    }
    printf("2-opt improving moves: %lld   best gain %d (a=%d c=%d)\n", cnt2, best2, ba, bc);
    fflush(stdout);
    if(ba>=0){
        int b=nxt(ba), c=bc, d=nxt(c);
        long long oldlen=0; for(int i=0;i<N;i++) oldlen+=dst(tour[i],tour[(i+1)%N]);
        int pb=pos[b], pc=pos[c];
        if(pb<=pc){ for(int i=pb,j=pc;i<j;i++,j--){int t=tour[i];tour[i]=tour[j];tour[j]=t;} }
        else { for(int i=pc+1,j=pb-1;i<j;i++,j--){int t=tour[i];tour[i]=tour[j];tour[j]=t;} }
        for(int i=0;i<N;i++) pos[tour[i]]=i;
        long long newlen=0; for(int i=0;i<N;i++) newlen+=dst(tour[i],tour[(i+1)%N]);
        printf("verify best move: before %lld after %lld delta %lld (expected -%d)\n", oldlen,newlen,newlen-oldlen,best2);
        fflush(stdout);
        /* restore */
        if(pb<=pc){ for(int i=pb,j=pc;i<j;i++,j--){int t=tour[i];tour[i]=tour[j];tour[j]=t;} }
        else { for(int i=pc+1,j=pb-1;i<j;i++,j--){int t=tour[i];tour[i]=tour[j];tour[j]=t;} }
        for(int i=0;i<N;i++) pos[tour[i]]=i;
    }

    /* ---- Or-opt scan (L=1..3, preserved or reversed) ---- */
    long long cnto=0; int besto=0;
    for(int i=0;i<N;i++){
        for(int L=1;L<=25;L++){
            int s1=tour[i];
            int e=(i+L-1)%N; int sL=tour[e];
            int p=tour[(i-1+N)%N];
            int q=tour[(i+L)%N];
            int dpq=dst(p,q);
            int oldpq = dst(p,s1)+dst(sL,q);
            for(int side=0; side<2; side++){
                int base = side? sL : s1;
                for(int t=0;t<nbc[base];t++){
                    int c = nb[base][t];
                    int off=pos[c]-i; if(off<0) off+=N; if(off<L) continue;
                    int d=nxt(c);
                    int offd=pos[d]-i; if(offd<0) offd+=N; if(offd<L) continue;
                    int old = oldpq + dst(c,d);
                    int g1 = old - (dpq + dst(c,s1) + dst(sL,d));
                    int g2 = old - (dpq + dst(c,sL) + dst(s1,d));
                    int gg = g1>g2? g1:g2;
                    if(gg>0){ cnto++; if(gg>besto) besto=gg; }
                }
            }
        }
    }
    printf("Or-opt(1..25) improving moves: %lld   best gain %d\n", cnto, besto); fflush(stdout);
    return 0;
}
