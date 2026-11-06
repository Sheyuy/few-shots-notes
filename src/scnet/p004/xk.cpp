// Complete k-exchange check on the 367-point set: independent A (|A|=K+1) outside S
// with |T(A)| <= K, where T(v) = S-neighbours of v.  K given on the command line.
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
struct Ent{int v;int n;vector<int> t;};
int K;
vector<int> uni(const vector<int>&c,const vector<int>&t){
    vector<int> o; size_t i=0,j=0;
    while(i<c.size()||j<t.size()){
        int v;
        if(j>=t.size()||(i<c.size()&&c[i]<t[j]))v=c[i++];
        else if(i>=c.size()||t[j]<c[i])v=t[j++];
        else {v=c[i++];j++;}
        o.push_back(v); if((int)o.size()>K) return o;
    }
    return o;
}
int main(int argc,char**argv){
    const char* path="/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt";
    K=argc>1?atoi(argv[1]):3;
    long long cap=argc>2?atoll(argv[2]):200000000LL;
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r"); if(!f)return 1;
    vector<int> S; int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5) S.push_back(enc(a));
    fclose(f);
    if((int)S.size()!=367){printf("bad S %zu\n",S.size());return 1;}
    vector<char> inS(N5,0); for(int s:S) inS[s]=1;
    vector<Ent> L;
    for(int v=0;v<N5;v++){
        if(inS[v])continue;
        vector<int> t; bool ok=true;
        for(int s:S) if(adj(v,s)){ t.push_back(s); if((int)t.size()>K){ok=false;break;} }
        if(ok) L.push_back({v,(int)t.size(),t});
    }
    printf("K=%d |A|=%d L=%zu\n",K,K+1,L.size());
    int m=L.size();
    vector<int> cur; vector<int> curT; long long nodes=0; bool found=false, stop=false;
    auto t0=chrono::steady_clock::now();
    function<void(int)> dfs=[&](int start){
        if(stop)return;
        if(++nodes>cap){stop=true;return;}
        if(nodes%5000000==0){fprintf(stderr,"progress nodes=%lld\n",nodes);fflush(stderr);}
        if((int)cur.size()==K+1){found=true;stop=true;return;}
        for(int i=start;i<m&&!stop;i++){
            bool ok=true;
            for(int v:cur) if(adj(v,L[i].v)){ok=false;break;}
            if(!ok)continue;
            vector<int> u=uni(curT,L[i].t);
            if((int)u.size()>K)continue;
            vector<int> save=curT; curT=u;
            cur.push_back(L[i].v);
            dfs(i+1);
            cur.pop_back(); curT=save;
        }
    };
    dfs(0);
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    if(found){
        printf("FOUND_%d_EXCHANGE:",K);
        for(int v:cur){printf(" [");for(int d=0;d<5;d++)printf("%d%s",dig[v][d],d<4?" ":" ");printf("]");}
        printf("\nT={"); for(int s:curT)printf("%d,",s); printf("}\n");
    } else if(stop){
        printf("CAP_REACHED nodes=%lld seconds=%.1f\n",nodes,sec);
    } else {
        printf("NO_%d_EXCHANGE nodes=%lld seconds=%.1f\n",K,nodes,sec);
    }
    return 0;
}
