#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define N 6561
#define MAXS 700
static uint8_t ADDHI[81][81], ADDLO[81][81], NEGHI[81], NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N]; static int conf[N]; static int sz;
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static void remp(int p){int pos=-1;for(int i=0;i<sz;i++)if(S[i]==p){pos=i;break;}S[pos]=S[--sz];inS[p]=0;for(int i=0;i<sz;i++)conf[third(p,S[i])]--;}

int main(void){
    init();
    FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1) addp(v); fclose(f);
    printf("size %d\n",sz);
    int best=0; int ba=-1,bb=-1;
    long cnt3=0;
    int cand[MAXS],ncand;
    for(int ia=0; ia<sz; ia++){
        int a=S[ia]; remp(a);
        for(int ib=0; ib<sz; ib++){
            int b=S[ib]; remp(b);
            ncand=0;
            for(int p=0;p<N;p++) if(!inS[p]&&conf[p]==0){ if(ncand<MAXS) cand[ncand]=p; ncand++; }
            if(ncand>best){best=ncand;ba=a;bb=b;}
            if(ncand>=3){
                cnt3++;
                if(cnt3<=5) printf("pair (%d,%d): addable %d\n",a,b,ncand);
            }
            addp(b);
        }
        addp(a);
    }
    printf("max addable after removing a pair: %d (a=%d b=%d), pairs with >=3: %ld\n",best,ba,bb,cnt3);
    return 0;
}
