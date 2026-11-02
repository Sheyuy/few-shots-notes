#define KK 13
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define N 6561
#define MAXS 600
static uint8_t ADDHI[81][81], NEGHI[81], NEGLO[81];
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){int hi=ADDHI[p/81][q/81]; int lo=add4(p%81,q%81); return neg4(hi)*81+neg4(lo);}
static uint8_t inS[N]; static int conf[N]; static uint16_t S[MAXS]; static int sz;
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++)ADDHI[a][b]=add4(a,b);}}
int main(void){
    init();
    FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1){S[sz++]=v;inS[v]=1;} fclose(f);
    for(int i=0;i<sz;i++)for(int j=i+1;j<sz;j++)conf[third(S[i],S[j])]++;
    // build pairs per outside point
    static uint32_t PA[N][120]; static uint8_t PN[N];
    for(int x=0;x<N;x++){ if(inS[x])continue; int n=0;
        for(int i=0;i<sz;i++){int a=S[i]; if(a==x)continue; int b=third(a,x); if(b!=a&&inS[b]){ if(a<b && n<120) PA[x][n++]=(uint32_t)a*8192+b; }}
        PN[x]=n;
    }
    // greedy matching per point, collect k=10
    static uint32_t MATCH[N][16]; static uint8_t MK[N];
    int k10[2000], n10=0;
    for(int x=0;x<N;x++){ if(inS[x])continue; uint8_t used[N/8+1]; memset(used,0,sizeof(used)); int m=0;
        for(int i=0;i<PN[x];i++){int a=PA[x][i]/8192,b=PA[x][i]%8192; if(!(used[a>>3]&(1<<(a&7)))&&!(used[b>>3]&(1<<(b&7)))){used[a>>3]|=1<<(a&7);used[b>>3]|=1<<(b&7); MATCH[x][m++]=(uint32_t)a*8192+b; if(m>=16)break;}}
        MK[x]=m; if(m==KK) k10[n10++]=x;
    }
    printf("k10 points %d\n",n10); fflush(stdout);
    long best=0; int bx=-1;
    for(int t=0;t<n10;t++){ int x=k10[t];
        for(int mask=0;mask<(1<<KK);mask++){
            int R[16];
            for(int i=0;i<KK;i++){int a=MATCH[x][i]/8192,b=MATCH[x][i]%8192; R[i]=(mask>>i&1)?a:b;}
            for(int i=0;i<KK;i++){int r=R[i]; for(int j=0;j<sz;j++){int q=S[j]; if(q!=r) conf[third(r,q)]--;}}
            long cnt=0;
            for(int p=0;p<N;p++) if(!inS[p] && conf[p]==0) cnt++;
            for(int i=0;i<KK;i++){int r=R[i]; for(int j=0;j<sz;j++){int q=S[j]; if(q!=r) conf[third(r,q)]++;}}
            if(cnt>best){best=cnt;bx=x;printf("best %ld x=%d mask=%d\n",best,x,mask);fflush(stdout);}
        }
    }
    printf("FINAL max addable after 10-removal: %ld (x=%d)\n",best,bx);
    return 0;
}
