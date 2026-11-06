// Complete k=4 exchange check: independent A (|A|=5) outside S with |T(A)|<=4.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
using namespace std;
static const int N5=16807;
int dig[N5][5];
static inline bool adj(int a,int b){
    bool any=false;
    for(int i=0;i<5;i++){int d=dig[a][i]-dig[b][i];if(d<0)d=-d;if(d==6)d=1;if(d>1)return false;if(d==1)any=true;}
    return any;
}
static inline int enc(const int v[5]){int x=0;for(int i=4;i>=0;i--)x=x*7+v[i];return x;}
static inline uint64_t keyN(const int*t,int n){
    uint64_t k=0; for(int i=0;i<n;i++)k=k*368ull+(uint64_t)(t[i]+1);
    for(int i=n;i<4;i++)k=k*368ull;
    return k*8ull+n;
}
struct Ent{int v;int n;int t[4];};
int main(int argc,char**argv){
    const char* path = argc>1?argv[1]:"/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt";
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r"); if(!f)return 1;
    vector<int> S; int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5) S.push_back(enc(a));
    fclose(f);
    vector<char> inS(N5,0); for(int s:S) inS[s]=1;
    long long cnt[7]={0};
    vector<Ent> L;
    for(int v=0;v<N5;v++){
        if(inS[v])continue;
        int t[4]={-1,-1,-1,-1}; int n=0;
        for(int s:S){ if(adj(v,s)){ if(n<4)t[n]=s; n++; if(n>4)break; } }
        if(n<=5)cnt[n]++;
        if(n<=4){ Ent e; e.v=v; e.n=n; for(int i=0;i<4;i++)e.t[i]=t[i]; L.push_back(e); }
    }
    printf("counts |T|=0..5: %lld %lld %lld %lld %lld %lld ; L=%zu\n",cnt[0],cnt[1],cnt[2],cnt[3],cnt[4],cnt[5],L.size());
    unordered_map<uint64_t, vector<int>> bucket;
    for(auto&e:L) bucket[keyN(e.t,e.n)].push_back(e.v);
    // dedupe unions U of size<=4
    unordered_set<uint64_t> seenU;
    vector<array<int,4>> Us; vector<int> Un;
    auto addU=[&](int*u,int n){
        int t[4]={-1,-1,-1,-1}; for(int i=0;i<n;i++)t[i]=u[i];
        uint64_t k=keyN(t,n);
        if(seenU.insert(k).second){ Us.push_back({t[0],t[1],t[2],t[3]}); Un.push_back(n); }
    };
    for(size_t i=0;i<L.size();i++) addU(L[i].t,L[i].n);
    long long pairs=0, ok=0;
    for(size_t i=0;i<L.size();i++)for(size_t j=i+1;j<L.size();j++){
        int u[4]; int n=0;
        int A[4],B[4]; for(int q=0;q<L[i].n;q++)A[q]=L[i].t[q]; for(int q=0;q<L[j].n;q++)B[q]=L[j].t[q];
        int x=0,y=0; n=0;
        while(x<L[i].n||y<L[j].n){
            int val;
            if(y>=L[j].n||(x<L[i].n&&A[x]<B[y]))val=A[x++];
            else if(x>=L[i].n||B[y]<A[x])val=B[y++];
            else {val=A[x++];y++;}
            u[n++]=val; if(n>4)break;
        }
        pairs++;
        if(n<=4) { addU(u,n); ok++; }
    }
    printf("pairs=%lld union<=4:%lld distinct U=%zu\n",pairs,ok,Us.size());
    // For each U, gather candidates and look for independent 5-set
    long long withFive=0;
    for(size_t ui=0;ui<Us.size();ui++){
        int U[4]; for(int q=0;q<4;q++)U[q]=Us[ui][q]; int n=Un[ui];
        vector<int> cand;
        for(int mask=0;mask<(1<<n);mask++){
            int t[4]={-1,-1,-1,-1}; int m=0;
            for(int b=0;b<n;b++) if(mask&(1<<b)) t[m++]=U[b];
            auto it=bucket.find(keyN(t,m));
            if(it!=bucket.end()) cand.insert(cand.end(),it->second.begin(),it->second.end());
        }
        if(cand.size()<5) continue;
        sort(cand.begin(),cand.end()); cand.erase(unique(cand.begin(),cand.end()),cand.end());
        if(cand.size()<5) continue;
        withFive++;
        int m=cand.size();
        // DFS for independent 5-set
        vector<int> cur;
        function<void(int)> dfs=[&](int start){
            if(cur.size()==5){
                printf("FOUND A5 with U={%d,%d,%d,%d}:",U[0],U[1],U[2],U[3]);
                for(int v:cur){printf(" [");for(int d=0;d<5;d++)printf("%d%s",dig[v][d],d<4?" ":" ");printf("]");}
                printf("\n"); exit(0);
            }
            for(int i=start;i<m;i++){
                bool ok2=true;
                for(int v:cur) if(adj(v,cand[i])){ok2=false;break;}
                if(!ok2) continue;
                cur.push_back(cand[i]); dfs(i+1); cur.pop_back();
            }
        };
        dfs(0);
    }
    printf("NO 4-EXCHANGE: U_checked_with>=5_candidates=%lld\n",withFive);
    return 0;
}
