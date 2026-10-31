#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define N 6561
#define MAXS 700
static uint8_t ADDHI[81][81],ADDLO[81][81],NEGHI[81],NEGLO[81];
static uint16_t S[MAXS]; static uint8_t inS[N]; static int conf[N]; static int sz;
static inline int add4(int a,int b){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;int b0=b/27,b1=(b/9)%3,b2=(b/3)%3,b3=b%3;return ((a0+b0)%3)*27+((a1+b1)%3)*9+((a2+b2)%3)*3+((a3+b3)%3);}
static inline int neg4(int a){int a0=a/27,a1=(a/9)%3,a2=(a/3)%3,a3=a%3;return ((3-a0)%3)*27+((3-a1)%3)*9+((3-a2)%3)*3+((3-a3)%3);}
static inline int third(int p,int q){return NEGHI[ADDHI[p/81][q/81]]*81+NEGLO[ADDLO[p%81][q%81]];}
static void init(void){for(int a=0;a<81;a++){NEGHI[a]=neg4(a);NEGLO[a]=neg4(a);for(int b=0;b<81;b++){ADDHI[a][b]=add4(a,b);ADDLO[a][b]=add4(a,b);}}}
static void addp(int p){inS[p]=1;S[sz++]=p;for(int i=0;i<sz-1;i++)conf[third(p,S[i])]++;}
static void remp(int p){int pos=-1;for(int i=0;i<sz;i++)if(S[i]==p){pos=i;break;}S[pos]=S[--sz];inS[p]=0;for(int i=0;i<sz;i++)conf[third(p,S[i])]--;}
int main(void){
    init(); FILE*f=fopen("baseline.idx","r"); int v; while(fscanf(f,"%d",&v)==1) addp(v); fclose(f);
    printf("size %d\n",sz);
    int found=0, neutral=0;
    for(int y=0;y<N;y++){
        if(inS[y]) continue;
        /* pairs */
        int pa[200],pb[200],np=0;
        for(int i=0;i<sz;i++){
            int a=S[i], b=third(y,a);
            if(inS[b] && a<b && np<200){pa[np]=a;pb[np]=b;np++;}
        }
        if(np==0){printf("ADDABLE %d\n",y); found=1; break;}
        /* intersection of all pairs */
        int common=-1;
        for(int t=0;t<2;t++){
            int cand = t? pb[0]:pa[0];
            int ok=1;
            for(int i=0;i<np;i++) if(pa[i]!=cand && pb[i]!=cand){ok=0;break;}
            if(ok){common=cand;break;}
        }
        if(common<0) continue;
        neutral++;
        /* swap: remove common, add y, then look for addable z */
        remp(common); addp(y);
        int z=-1;
        for(int q=0;q<N;q++) if(!inS[q] && conf[q]==0){z=q;break;}
        if(z>=0){
            printf("FOUND 513: remove %d add %d then add %d\n",common,y,z);
            addp(z);
            FILE*o=fopen("found513.idx","w"); for(int i=0;i<sz;i++) fprintf(o,"%d\n",S[i]); fclose(o);
            found=1; break;
        }
        /* also try adding any single point after removing common but before adding y? skip */
        remp(y); addp(common);
    }
    printf("neutral swaps: %d, found=%d\n",neutral,found);
    return 0;
}
