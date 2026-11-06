// Complete k-exchange check on the 367-point set (C7^5), allocation-free DFS.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
#include <functional>
using namespace std;
static const int N5=16807;
int dig[N5][5];
static inline bool adj(int a,int b){
    bool any=false;
    for(int i=0;i<5;i++){int d=dig[a][i]-dig[b][i];if(d<0)d=-d;if(d==6)d=1;if(d>1)return false;if(d==1)any=true;}
    return any;
}
static inline int enc(const int v[5]){int x=0;for(int i=4;i>=0;i--)x=x*7+v[i];return x;}
int K,m;
struct Ent{int v;int n;int t[16];};
vector<Ent> L;
int cur[16], curN=0, curT[16], curTn=0;
long long nodes=0, cap=0; bool found=false, stopped=false;
static inline int unionN(const int*c,int nc,const int*t,int nt,int*out){
    int i=0,j=0,n=0;
    while(i<nc||j<nt){
        int v;
        if(j>=nt||(i<nc&&c[i]<t[j]))v=c[i++];
        else if(i>=nc||t[j]<c[i])v=t[j++];
        else {v=c[i++];j++;}
        out[n++]=v; if(n>K)return n;
    }
    return n;
}
void dfs(int start){
    if(stopped)return;
    if(++nodes>cap){stopped=true;return;}
    if(nodes%2000000==0){fprintf(stderr,"progress nodes=%lld\n",nodes);fflush(stderr);}
    if(curN==K+1){found=true;stopped=true;return;}
    for(int i=start;i<m&&!stopped;i++){
        bool ok=true;
        for(int q=0;q<curN;q++) if(adj(cur[q],L[i].v)){ok=false;break;}
        if(!ok)continue;
        int u[16]; int nu=unionN(curT,curTn,L[i].t,L[i].n,u);
        if(nu>K)continue;
        int save[16]; int sn=curTn; for(int q=0;q<sn;q++)save[q]=curT[q];
        for(int q=0;q<nu;q++)curT[q]=u[q]; curTn=nu;
        cur[curN++]=L[i].v;
        dfs(i+1);
        curN--; curTn=sn; for(int q=0;q<sn;q++)curT[q]=save[q];
    }
}
int main(int argc,char**argv){
    const char* path="/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt";
    K=argc>1?atoi(argv[1]):3;
    cap=argc>2?atoll(argv[2]):2000000000LL;
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r"); if(!f)return 1;
    vector<int> S; int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5) S.push_back(enc(a));
    fclose(f);
    vector<char> inS(N5,0); for(int s:S) inS[s]=1;
    for(int v=0;v<N5;v++){
        if(inS[v])continue;
        Ent e; e.v=v; e.n=0; bool ok=true;
        for(int s:S) if(adj(v,s)){ if(e.n>=K+1){ok=false;break;} e.t[e.n++]=s; }
        if(ok) L.push_back(e);
    }
    m=L.size();
    fprintf(stderr,"K=%d |A|=%d L=%d\n",K,K+1,m); fflush(stderr);
    auto t0=chrono::steady_clock::now();
    dfs(0);
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    if(found){
        printf("FOUND_%d_EXCHANGE:",K);
        for(int q=0;q<curN;q++){printf(" [");for(int d=0;d<5;d++)printf("%d%s",dig[cur[q]][d],d<4?" ":" ");printf("]");}
        printf("\nT={");for(int q=0;q<curTn;q++)printf("%d,",curT[q]);printf("}\n");
    } else if(stopped){
        printf("CAP_REACHED nodes=%lld seconds=%.1f\n",nodes,sec);
    } else {
        printf("NO_%d_EXCHANGE nodes=%lld seconds=%.1f\n",K,nodes,sec);
    }
    return 0;
}
