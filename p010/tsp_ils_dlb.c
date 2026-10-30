#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define N 100000
#define K 20
#define GX 100
#define QCAP (N*8)

static int X[N], Y[N];
static int tour[N], pos[N], best[N], cur[N];
static long long curlen;
static long long bestlen;

static inline int dst(int i, int j) {
    int dx = X[i]-X[j], dy = Y[i]-Y[j];
    int s = dx*dx + dy*dy;
    int r = (int)sqrt((double)s);
    while ((long long)r*r > s) r--;
    while ((long long)(r+1)*(r+1) <= s) r++;
    return (s > r*r + r) ? r+1 : r;
}
static inline int at(int k) { return tour[k >= N ? k-N : k]; }
static inline int nxt(int a) { int p = pos[a]+1; return tour[p >= N ? 0 : p]; }

static int cellStart[GX*GX+1], cellItems[N], cellOf[N];
static int nb[N][K], nbc[N];

static unsigned long rs = 88172645463325252UL;
static unsigned int xr(void){ rs^=rs<<13; rs^=rs>>7; rs^=rs<<17; return (unsigned int)(rs>>11); }

static long long tlen(void) { long long s=0; for (int i=0;i<N;i++) s+=dst(tour[i],tour[(i+1)%N]); return s; }
static void setpos(void) { for (int i=0;i<N;i++) pos[tour[i]]=i; }

static int queue[QCAP]; static char inq[N]; static int qh, qt, qcnt;
static void push(int c) {
    if (inq[c]) return;
    if (qcnt >= QCAP) return;
    inq[c]=1; queue[qt]=c; qt=(qt+1)%QCAP; qcnt++;
}
static int pop(void) {
    if (qcnt==0) return -1;
    int c=queue[qh]; qh=(qh+1)%QCAP; qcnt--; inq[c]=0; return c;
}

static void rev_range(int i,int j){ while(i<j){int t=tour[i];tour[i]=tour[j];tour[j]=t;i++;j--;} }
static void fixpos(int i,int j){ for(int k=i;k<=j;k++) pos[tour[k]]=k; }

static int apply_2opt(int a,int c){
    int b=nxt(a), d=nxt(c);
    if (b==c||d==a||b==d) return 0;
    int pb=pos[b], pc=pos[c];
    if (pb<=pc){ rev_range(pb,pc); fixpos(pb,pc); }
    else { rev_range(pc+1,pb-1); fixpos(pc+1,pb-1); }
    return 1;
}

static int tmp[N];
static int apply_oropt(int i,int L,int c,int rev){
    int seg[4];
    for(int k=0;k<L;k++) seg[k]=tour[(i+k)%N];
    if(rev){ for(int k=0;k<L/2;k++){int t=seg[k];seg[k]=seg[L-1-k];seg[L-1-k]=t;} }
    int m=0;
    for(int k=0;k<N;k++){ int idx=(i+k)%N; if(k<L) continue; tmp[m++]=tour[idx]; }
    int ci=-1; for(int k=0;k<m;k++) if(tmp[k]==c){ci=k;break;}
    if(ci<0) return 0;
    int w=0;
    for(int k=0;k<=ci;k++) tour[w++]=tmp[k];
    for(int k=0;k<L;k++) tour[w++]=seg[k];
    for(int k=ci+1;k<m;k++) tour[w++]=tmp[k];
    setpos();
    return 1;
}

static const int MATCH[7][3][2] = {
    {{0,1},{2,4},{3,5}}, {{0,2},{1,3},{4,5}}, {{0,2},{1,4},{3,5}},
    {{0,3},{1,4},{2,5}}, {{0,3},{1,5},{2,4}}, {{0,4},{1,3},{2,5}},
    {{0,4},{1,5},{2,3}}
};
static const int SEGPAR[6] = {5,2,1,4,3,0};
static int eidx[6], newpar[6], order_buf[N];

static void append_path(int *m,int x,int y,int relC,int relE){
    if (x==1&&y==2){ for(int k=2;k<=relC;k++) order_buf[(*m)++]=at(k); }
    else if (x==2&&y==1){ for(int k=relC-1;k>=1;k--) order_buf[(*m)++]=at(k); }
    else if (x==3&&y==4){ for(int k=relC+2;k<=relE;k++) order_buf[(*m)++]=at(k); }
    else if (x==4&&y==3){ for(int k=relE-1;k>=relC+1;k--) order_buf[(*m)++]=at(k); }
    else if (x==5&&y==0){ for(int k=relE+2;k<=N-1;k++) order_buf[(*m)++]=at(k); order_buf[(*m)++]=at(0); }
    else if (x==0&&y==5){ for(int k=N-1;k>=relE+1;k--) order_buf[(*m)++]=at(k); }
}

static int apply_3opt(int A,int C,int E,int cs){
    int pA=pos[A];
    int relC=(pos[C]-pA+N)%N;
    int relE=(pos[E]-pA+N)%N;
    if (!(0<relC && relC+1<relE && relE+1<=N-1)) return 0;
    eidx[0]=A; eidx[1]=at(1); eidx[2]=at(relC); eidx[3]=at(relC+1); eidx[4]=at(relE); eidx[5]=at(relE+1);
    for(int i=0;i<6;i++) newpar[i]=-1;
    for(int t=0;t<3;t++){ int u=MATCH[cs][t][0], v=MATCH[cs][t][1]; newpar[u]=v; newpar[v]=u; }
    int m=0; order_buf[m++]=A; int cur=0;
    for(int g=0; g<10; g++){
        int p=newpar[cur];
        if(p==0) break;
        order_buf[m++]=eidx[p];
        int q=SEGPAR[p];
        append_path(&m,p,q,relC,relE);
        if(q==0) break;
        cur=q;
    }
    if(m!=N) return 0;
    memcpy(tour,order_buf,N*sizeof(int));
    setpos();
    return 1;
}

static int try_city(int a){
    int improved=0;
    {
        int b=nxt(a), dab=dst(a,b);
        for(int t=0;t<nbc[a];t++){
            int c=nb[a][t]; int dac=dst(a,c);
            if(dac>=dab) break;
            int d=nxt(c);
            if(c==a||d==a||c==b||d==b) continue;
            if(dab+dst(c,d)-dac-dst(b,d)>0 && apply_2opt(a,c)){
                push(a);push(b);push(c);push(d); improved=1;
                b=nxt(a); dab=dst(a,b); t=-1;
            }
        }
    }
    {
        int i=pos[a];
        for(int L=1;L<=3;L++){
            int s1=tour[i], sL=tour[(i+L-1)%N];
            int p=tour[(i-1+N)%N], q=tour[(i+L)%N];
            int dpq=dst(p,q), oldpq=dst(p,s1)+dst(sL,q);
            int bg=0,bc=-1,brev=0;
            for(int side=0;side<2;side++){
                int base=side?sL:s1;
                for(int t=0;t<nbc[base];t++){
                    int c=side?nb[sL][t]:nb[s1][t];
                    int off=pos[c]-i; if(off<0) off+=N; if(off<L) continue;
                    int d=nxt(c);
                    int offd=pos[d]-i; if(offd<0) offd+=N; if(offd<L) continue;
                    int old=oldpq+dst(c,d);
                    int g1=old-(dpq+dst(c,s1)+dst(sL,d));
                    int g2=old-(dpq+dst(c,sL)+dst(s1,d));
                    int g=g1>g2?g1:g2;
                    if(g>bg){bg=g;bc=c;brev=(g2>g1);}
                }
            }
            if(bg>0 && apply_oropt(i,L,bc,brev)){ push(a);push(p);push(q);push(bc);push(nxt(bc)); improved=1; break; }
        }
    }
    if(!improved){
        int A=a, B=nxt(A); int dAB=dst(A,B);
        for(int ci=0;ci<nbc[B];ci++){
            int C=nb[B][ci];
            int oC=pos[C]-pos[A]; if(oC<0) oC+=N;
            if(oC<2||oC>N-3) continue;
            int D=nxt(C); int dCD=dst(C,D);
            for(int ei=0;ei<nbc[D];ei++){
                int E=nb[D][ei];
                int oE=pos[E]-pos[A]; if(oE<0) oE+=N;
                if(oE<oC+2||oE>N-2) continue;
                int F=nxt(E);
                if(F==A||E==A||C==A||D==A) continue;
                int dEF=dst(E,F);
                int node[6]={A,B,C,D,E,F};
                int base=dAB+dCD+dEF;
                for(int cs=0;cs<7;cs++){
                    int sum=dst(node[MATCH[cs][0][0]],node[MATCH[cs][0][1]])
                           +dst(node[MATCH[cs][1][0]],node[MATCH[cs][1][1]])
                           +dst(node[MATCH[cs][2][0]],node[MATCH[cs][2][1]]);
                    if(base-sum>0 && apply_3opt(A,C,E,cs)){
                        push(A);push(B);push(C);push(D);push(E);push(F);
                        improved=1; ci=nbc[B]; break;
                    }
                }
            }
        }
    }
    return improved;
}

static void repair(void){
    int c; long long guard = 0;
    while((c=pop())>=0){
        try_city(c);
        if (++guard > 300000) break;      /* safety valve */
    }
}

static int GAP = 1000;
static int TRAVEL = 100;
static void double_bridge(int *pi,int *pj,int *pk){
    int i = 1 + xr() % (N-3000);
    int j = i + 1 + xr() % GAP;
    int k = j + 1 + xr() % GAP;
    if (k >= N-1) k = N-2;
    int m=0;
    for(int t=0;t<=i;t++) tmp[m++]=tour[t];
    for(int t=j+1;t<=k;t++) tmp[m++]=tour[t];
    for(int t=i+1;t<=j;t++) tmp[m++]=tour[t];
    for(int t=k+1;t<N;t++) tmp[m++]=tour[t];
    memcpy(tour,tmp,N*sizeof(int));
    setpos();
    *pi=i; *pj=j; *pk=k;
}

int main(int argc,char**argv){
    int seconds=(argc>1)?atoi(argv[1]):600;
    unsigned long seed=(argc>2)?(unsigned long)atoll(argv[2]):12345;
    rs=seed?seed:1;
    if(argc>3){ GAP=atoi(argv[3]); if(GAP<10)GAP=10; if(GAP>N-3000)GAP=N-3000; }
    int nbridge=(argc>4)?atoi(argv[4]):1; if(nbridge<1) nbridge=1;
    if(argc>5){ TRAVEL=atoi(argv[5]); if(TRAVEL<0)TRAVEL=0; }
    FILE *f=fopen("coords.txt","r"); if(!f){fprintf(stderr,"no coords\n");return 1;}
    for(int i=0;i<N;i++) if(fscanf(f,"%d %d",&X[i],&Y[i])!=2) return 1;
    fclose(f);
    FILE *g=fopen("baseline_tour.txt","r"); if(!g){fprintf(stderr,"no tour\n");return 1;}
    for(int i=0;i<N;i++) if(fscanf(g,"%d",&tour[i])!=1) return 1;
    fclose(g);
    setpos();
    int mnx=1<<30,mny=1<<30,mxx=-(1<<30),mxy=-(1<<30);
    for(int i=0;i<N;i++){ if(X[i]<mnx)mnx=X[i]; if(X[i]>mxx)mxx=X[i]; if(Y[i]<mny)mny=Y[i]; if(Y[i]>mxy)mxy=Y[i]; }
    double cw=(double)(mxx-mnx+1)/GX, ch=(double)(mxy-mny+1)/GX;
    static int cnt[GX*GX];
    for(int i=0;i<N;i++){ int cx=(int)((X[i]-mnx)/cw); if(cx>=GX)cx=GX-1; int cy=(int)((Y[i]-mny)/ch); if(cy>=GX)cy=GX-1; cellOf[i]=cy*GX+cx; cnt[cellOf[i]]++; }
    int s=0; for(int c=0;c<GX*GX;c++){ cellStart[c]=s; s+=cnt[c]; } cellStart[GX*GX]=s;
    static int fillc[GX*GX];
    for(int i=0;i<N;i++){ int c=cellOf[i]; cellItems[cellStart[c]+fillc[c]++]=i; }
    for(int i=0;i<N;i++){
        int cx=(int)((X[i]-mnx)/cw); if(cx>=GX)cx=GX-1;
        int cy=(int)((Y[i]-mny)/ch); if(cy>=GX)cy=GX-1;
        int found=0,r=0;
        while(found<K && r<GX){
            for(int yy=cy-r;yy<=cy+r;yy++){ if(yy<0||yy>=GX)continue;
                for(int xx=cx-r;xx<=cx+r;xx++){ if(xx<0||xx>=GX)continue;
                    if((abs(xx-cx)!=r)&&(abs(yy-cy)!=r))continue;
                    int c=yy*GX+xx;
                    for(int t=cellStart[c];t<cellStart[c+1];t++){ int j=cellItems[t]; if(j==i)continue; if(found<K) nb[i][found++]=j; }
                }
            }
            r++;
        }
        nbc[i]=found;
        for(int a=1;a<found;a++){ int vj=nb[i][a], vd=dst(i,vj), b=a-1;
            while(b>=0 && dst(i,nb[i][b])>vd){ nb[i][b+1]=nb[i][b]; b--; }
            nb[i][b+1]=vj; }
    }
    bestlen=tlen(); memcpy(best,tour,N*sizeof(int));
    memcpy(cur,tour,N*sizeof(int)); curlen=bestlen;
    printf("start len=%lld\n",bestlen); fflush(stdout);

    clock_t t0=clock();
    long long kicks=0, accepted=0;
    while((double)(clock()-t0)/CLOCKS_PER_SEC < seconds){
        memcpy(tour,cur,N*sizeof(int)); setpos();
        int i=0,j=0,k=0;
        for(int b=0;b<nbridge;b++) double_bridge(&i,&j,&k);
        push(tour[i]); push(tour[(i+1)%N]);
        push(tour[j]); push(tour[(j+1)%N]);
        push(tour[k]); push(tour[(k+1)%N]);
        push(tour[0]); push(tour[N-1]);
        repair();
        kicks++;
        long long L=tlen();
        if(L<bestlen){
            bestlen=L; memcpy(best,tour,N*sizeof(int)); accepted++;
            printf("  *** NEW BEST %lld at kick %lld ***\n",bestlen,kicks); fflush(stdout);
        }
        if(L <= bestlen + TRAVEL){ memcpy(cur,tour,N*sizeof(int)); curlen=L; }
        else { memcpy(cur,best,N*sizeof(int)); curlen=bestlen; }
        if(kicks%200==0){ printf("kicks=%lld best=%lld accepted=%lld\n",kicks,bestlen,accepted); fflush(stdout); }
    }
    printf("done kicks=%lld accepted=%lld best=%lld\n",kicks,accepted,bestlen);
    FILE *o=fopen("ils_best_tour.txt","w");
    for(int i=0;i<N;i++) fprintf(o,"%d\n",best[i]);
    fclose(o);
    return 0;
}
