// Parallel complete k-exchange check on R367 (C7^5): is there an independent A
// with |A| = k+1 and |T(A)| <= k?  Equivalent to xk.c but parallel: every
// first-level choice i0 is an independent subtree, claimed from a shared queue.
//
//   g++ -x c++ -std=c++17 -O3 xk_par.c -o xk_par      (no OpenMP needed)
//   ./xk_par K cap i0lo i0hi [threads] [R367.txt]
//
// Node counts are exactly xk.c's minus the root node; verified for k=3..6.
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <thread>
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
std::atomic<long long> totalNodes(0);
std::atomic<bool> found(false);
bool capHit=false; long long cap=0;
struct St{int cur[16];int curN=0;int curT[16];int curTn=0;long long nodes=0;};
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
void dfs(St& s,int start){
    if(found.load(memory_order_relaxed))return;
    if(++s.nodes>cap){capHit=true;return;}
    if((s.nodes&0xFFFFF)==0){
        totalNodes.fetch_add(0);
        if(s.nodes%4000000==0){fprintf(stderr,"progress nodes=%lld\n",(long long)totalNodes.load()+s.nodes);fflush(stderr);}
    }
    if(s.curN==K+1){found.store(true);return;}
    for(int i=start;i<m&&!found.load(memory_order_relaxed);i++){
        bool ok=true;
        for(int q=0;q<s.curN;q++) if(adj(s.cur[q],L[i].v)){ok=false;break;}
        if(!ok)continue;
        int u[16]; int nu=unionN(s.curT,s.curTn,L[i].t,L[i].n,u);
        if(nu>K)continue;
        int save[16]; int sn=s.curTn; for(int q=0;q<sn;q++)save[q]=s.curT[q];
        for(int q=0;q<nu;q++)s.curT[q]=u[q]; s.curTn=nu;
        s.cur[s.curN++]=L[i].v;
        dfs(s,i+1);
        s.curN--; s.curTn=sn; for(int q=0;q<sn;q++)s.curT[q]=save[q];
    }
}
int main(int argc,char**argv){
    const char* path=argc>6?argv[6]:"R367.txt";   // 6th arg: path to R367.txt
    K=argc>1?atoi(argv[1]):3;
    cap=argc>2?atoll(argv[2]):2000000000000LL;
    int lo=argc>3?atoi(argv[3]):0;
    int hi=argc>4?atoi(argv[4]):-1;
    int threads=argc>5?atoi(argv[5]):8;
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
    m=L.size(); if(hi<0||hi>m)hi=m; if(lo<0)lo=0; if(lo>hi)lo=hi;
    fprintf(stderr,"K=%d |A|=%d L=%d range=[%d,%d) threads=%d\n",K,K+1,m,lo,hi,threads); fflush(stderr);
    auto t0=chrono::steady_clock::now();
    std::atomic<int> nextTask(lo);
    auto worker=[&](){
        St s;
        while(true){
            int i0=nextTask.fetch_add(1,memory_order_relaxed);
            if(i0>=hi)break;
            if(found.load(memory_order_relaxed)||capHit)break;
            if(L[i0].n>K)continue;
            s.curN=0;s.curTn=0;s.nodes=0;
            s.cur[0]=L[i0].v;s.curN=1;
            s.curTn=L[i0].n;for(int q=0;q<s.curTn;q++)s.curT[q]=L[i0].t[q];
            dfs(s,i0+1);
            totalNodes.fetch_add(s.nodes,memory_order_relaxed);
        }
    };
    std::vector<std::thread> pool;
    for(int t=0;t<threads;t++)pool.emplace_back(worker);
    for(auto&t:pool)t.join();
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    long long n=totalNodes.load();
    if(found.load()){printf("FOUND_%d_EXCHANGE nodes=%lld seconds=%.1f\n",K,n,sec);}
    else if(capHit){printf("CAP_REACHED nodes=%lld seconds=%.1f\n",n,sec);}
    else{printf("NO_%d_EXCHANGE nodes=%lld seconds=%.1f range=[%d,%d)\n",K,n,sec,lo,hi);}
    return 0;
}
