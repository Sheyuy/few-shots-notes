// Complete k=4 exchange: independent A (|A|=5) outside S with |T(A)| <= 4.
// DFS over vertices with |T(v)|<=4, enforcing |union of T| <= 4 at every step.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;
static const int N5=16807;
int dig[N5][5];
static inline bool adj(int a,int b){
    bool any=false;
    for(int i=0;i<5;i++){int d=dig[a][i]-dig[b][i];if(d<0)d=-d;if(d==6)d=1;if(d>1)return false;if(d==1)any=true;}
    return any;
}
static inline int enc(const int v[5]){int x=0;for(int i=4;i>=0;i--)x=x*7+v[i];return x;}
static inline int uni(int*c,int nc,const int*t,int nt,int*out){
    int i=0,j=0,n=0;
    while(i<nc||j<nt){
        int v;
        if(j>=nt||(i<nc&&c[i]<t[j]))v=c[i++];
        else if(i>=nc||t[j]<c[i])v=t[j++];
        else {v=c[i++];j++;}
        out[n++]=v; if(n>4)return n;
    }
    return n;
}
struct Ent{int v;int n;int t[4];};
int main(int argc,char**argv){
    const char* path = argc>1?argv[1]:"/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt";
    long long cap = argc>2?atoll(argv[2]):200000000LL;
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r"); if(!f)return 1;
    vector<int> S; int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5) S.push_back(enc(a));
    fclose(f);
    vector<char> inS(N5,0); for(int s:S) inS[s]=1;
    vector<Ent> L;
    for(int v=0;v<N5;v++){
        if(inS[v])continue;
        int t[4]={-1,-1,-1,-1}; int n=0;
        for(int s:S){ if(adj(v,s)){ if(n<4)t[n]=s; n++; if(n>4)break; } }
        if(n<=4){ Ent e; e.v=v; e.n=n; for(int i=0;i<4;i++)e.t[i]=t[i]; L.push_back(e); }
    }
    printf("L=%zu\n",L.size());
    int m=L.size(); vector<int> cur; long long nodes=0; bool stop=false;
    auto t0=chrono::steady_clock::now();
    vector<int> curT; // union
    function<void(int)> dfs=[&](int start){
        if(stop)return;
        if(++nodes>cap){stop=true;return;}
        if(cur.size()==5){
            printf("FOUND A5:");
            for(int v:cur){printf(" [");for(int d=0;d<5;d++)printf("%d%s",dig[v][d],d<4?" ":" ");printf("]");}
            printf("\n"); stop=true; return;
        }
        for(int i=start;i<m&&!stop;i++){
            bool ok=true;
            for(int v:cur) if(adj(v,L[i].v)){ok=false;break;}
            if(!ok)continue;
            int u[8]; int nu=uni(curT.data(),(int)curT.size(),L[i].t,L[i].n,u);
            if(nu>4)continue;
            vector<int> saveT=curT; curT.assign(u,u+nu);
            cur.push_back(L[i].v);
            dfs(i+1);
            cur.pop_back(); curT=saveT;
        }
    };
    dfs(0);
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("%s nodes=%lld seconds=%.1f\n", stop&&cur.size()!=5?"STOPPED_OR_FOUND":"NO_4_EXCHANGE", nodes, sec);
    return 0;
}
