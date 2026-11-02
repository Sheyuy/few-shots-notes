#include <stdio.h>
#include <stdint.h>
#define N 6561
static uint8_t ADDHI[81][81],ADDLO[81][81],NEGHI[81],NEGLO[81];
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
int main(void){
    for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}
    printf("third(100,200)=%d third(0,1)=%d third(6560,6560)=%d\n",third(100,200),third(0,1),third(6560,6560));
    static int conf[N]; static uint8_t inS[N];
    FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1) inS[v]=1;
    fclose(f);
    int S[700],n=0; for(int i=0;i<N;i++) if(inS[i]) S[n++]=i;
    for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)conf[third(S[i],S[j])]++;
    int bad=0,maxin=0; for(int i=0;i<n;i++){if(conf[S[i]]>maxin)maxin=conf[S[i]]; if(conf[S[i]]!=0)bad++;}
    int minout=1000; for(int i=0;i<N;i++) if(!inS[i] && conf[i]<minout) minout=conf[i];
    printf("n=%d in-cap points with conf!=0: %d (max %d), min conf outside: %d\n",n,bad,maxin,minout);
    return 0;
}
