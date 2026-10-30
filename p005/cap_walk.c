#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define M8 6561
#define N0 512
#define MAXN 2048

static short *third8;
static int pow3[9];

static unsigned long st;
static unsigned int xr(void){st^=st<<13;st^=st>>7;st^=st<<17;return (unsigned int)(st>>11);}

static int S[MAXN], n;
static char inS[M8];
static int bc[M8];             /* bc[q] = # pairs {a,b} in S with third(a,b)=q */
static int Sbase[MAXN], nbase;
static int bcbase[M8];

static inline void addpt(int p){
    for(int j=0;j<n;j++) bc[third8[(size_t)p*M8+S[j]]]++;
    S[n++]=p; inS[p]=1;
}
static inline void rempt(int a){
    for(int j=0;j<n;j++){ int b=S[j]; if(b==a) continue; bc[third8[(size_t)a*M8+b]]--; }
    int pos=-1; for(int j=0;j<n;j++) if(S[j]==a){pos=j;break;}
    S[pos]=S[n-1]; n--; inS[a]=0;
}
static void savebase(void){ memcpy(Sbase,S,n*sizeof(int)); nbase=n; memcpy(bcbase,bc,sizeof(bc)); }
static void restorebase(void){ memcpy(S,Sbase,nbase*sizeof(int)); n=nbase; memset(inS,0,M8); for(int i=0;i<n;i++) inS[S[i]]=1; memcpy(bc,bcbase,sizeof(bc)); }

static int trywin(const char*out){
    FILE *o=fopen(out,"w");
    fprintf(stderr,"WIN n=%d\n",n);
    for(int i=0;i<n;i++){ int x=S[i]; int v[8]; for(int k=0;k<8;k++){v[k]=x%3;x/=3;}
        fprintf(o,"%d %d %d %d %d %d %d %d\n",v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7]); }
    fclose(o);
    return 1;
}

int main(int argc,char**argv){
    long iters = (argc>1)? atol(argv[1]) : 1000000;
    unsigned long seed = (argc>2)? (unsigned long)atoll(argv[2]) : 1UL;
    int kmax = (argc>3)? atoi(argv[3]) : 1;   /* max removals per kick (1 or 2) */
    const char *out = (argc>4)? argv[4] : "candidate_walk.txt";
    st = seed?seed:1;
    pow3[0]=1; for(int k=1;k<9;k++) pow3[k]=pow3[k-1]*3;
    static unsigned char digs[M8][8];
    for(int x=0;x<M8;x++){ int t=x; for(int k=0;k<8;k++){ digs[x][k]=t%3; t/=3; } }
    third8=malloc((size_t)M8*M8*sizeof(short));
    if(!third8){fprintf(stderr,"alloc\n");return 1;}
    for(int a=0;a<M8;a++) for(int b=0;b<M8;b++){
        int z=0; for(int k=0;k<8;k++){ int s=(digs[a][k]+digs[b][k])%3; z+=((3-s)%3)*pow3[k]; }
        third8[(size_t)a*M8+b]=(short)z;
    }
    FILE *f=fopen("baseline_pts.txt","r"); if(!f){fprintf(stderr,"no baseline\n");return 1;}
    n=0; memset(inS,0,M8); memset(bc,0,M8);
    for(int t=0;t<N0;t++){
        int v[8]; for(int k=0;k<8;k++) if(fscanf(f,"%d",&v[k])!=1){fprintf(stderr,"parse\n");return 1;}
        int c=0; for(int k=0;k<8;k++) c+=v[k]*pow3[k];
        S[n++]=c; inS[c]=1;
    }
    fclose(f);
    for(int i=0;i<n;i++) for(int j=i+1;j<n;j++) bc[third8[(size_t)S[i]*M8+S[j]]]++;
    savebase();
    printf("start n=%d seed=%lu kmax=%d\n",n,seed,kmax); fflush(stdout);

    static int addlist[M8];
    int bestExtra=0;
    for(long it=0; it<iters; it++){
        int k = (kmax<=1)?1: (1 + (int)(xr()%(unsigned)(kmax<16?kmax:16)));
        int rem[16]; for(int q=0;q<k;q++){ int idx=xr()%(unsigned)n; int a=S[idx]; rem[q]=a; rempt(a); }
        /* collect addable */
        int na=0;
        for(int q=0;q<M8;q++) if(!inS[q] && bc[q]==0) addlist[na++]=q;
        if(na>bestExtra) bestExtra=na;
        /* shuffle */
        for(int i=na-1;i>0;i--){int j=xr()%(i+1);int t=addlist[i];addlist[i]=addlist[j];addlist[j]=t;}
        /* greedy add with recheck, up to several passes */
        int added=0;
        for(int pass=0; pass<2; pass++){
            int any=0;
            for(int i=0;i<na;i++){ int p=addlist[i]; if(inS[p]) continue; if(bc[p]==0){ addpt(p); added++; any=1; if(n>=513) break; } }
            if(!any) break;
            if(n>=513) break;
        }
        if(n>=513){ trywin(out); printf("WIN at iter %ld n=%d\n",it,n); return 0; }
        if(k>=2 && n<512){ restorebase(); continue; }   /* failed to rebuild a 512-cap */
        if(k==1 && added==0){ if(bc[rem[0]]==0) addpt(rem[0]); else restorebase(); }
        /* periodic direct maximality check: is current 512-cap non-maximal? */
        if((it & 127)==0){
            for(int q=0;q<M8;q++) if(!inS[q] && bc[q]==0){ addpt(q); trywin(out); printf("WIN(maximal) at iter %ld n=%d\n",it,n); return 0; }
        }
        if((it%500000)==0 && it>0){ printf("it=%ld n=%d bestExtra=%d\n",it,n,bestExtra); fflush(stdout); }
    }
    printf("no win; n=%d bestExtra=%d\n",n,bestExtra);
    return 0;
}
