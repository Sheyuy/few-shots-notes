// lns6: ruin-and-recreate local search for a 368-point independent set in C7^5.
// Heuristic: finds witnesses, never proves absence.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>
using namespace std;
static const int N5=16807;
static int dig[N5][5];
static vector<uint16_t> nb[N5];
static void builddig(){for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}}
static void buildnb(){
    for(int v=0;v<N5;v++){
        int d[5];for(int i=0;i<5;i++)d[i]=dig[v][i];
        int idx[5];vector<uint16_t> t;t.reserve(243);
        for(idx[0]=-1;idx[0]<=1;idx[0]++)for(idx[1]=-1;idx[1]<=1;idx[1]++)
        for(idx[2]=-1;idx[2]<=1;idx[2]++)for(idx[3]=-1;idx[3]<=1;idx[3]++)
        for(idx[4]=-1;idx[4]<=1;idx[4]++){
            bool any=false;int x=0;
            for(int i=4;i>=0;i--){int c=(d[i]+idx[i]+7)%7;if(idx[i])any=true;x=x*7+c;}
            if(any)t.push_back((uint16_t)x);
        }
        nb[v].swap(t);
    }
}
struct Rng{uint64_t s;explicit Rng(uint64_t x):s(x?x:88172645463325252ULL){}
    inline uint64_t next(){s^=s<<13;s^=s>>7;s^=s<<17;return s;}
    inline uint32_t below(uint32_t n){return (uint32_t)(next()%(uint64_t)n);}};
static bool independent(const vector<int>&v){
    vector<char> in(N5,0);for(int x:v)in[x]=1;
    for(int x:v)for(auto u:nb[x])if(u!=x&&in[u])return false;
    return true;
}
int main(int argc,char**argv){
    int threads=argc>1?atoi(argv[1]):8;
    long long chains=argc>2?atoll(argv[2]):100000;
    const char* path=argc>3?argv[3]:"R367.txt";
    int target=argc>4?atoi(argv[4]):368;
    int kmin=argc>5?atoi(argv[5]):8, kmax=argc>6?atoi(argv[6]):20;
    builddig();buildnb();
    FILE*f=fopen(path,"r");if(!f){printf("NO_FILE %s\n",path);return 1;}
    vector<int> S;int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5){int x=0;for(int i=4;i>=0;i--)x=x*7+a[i];S.push_back(x);}
    fclose(f);
    if(!independent(S)){printf("BAD_INPUT\n");return 1;}
    printf("START size=%zu target=%d threads=%d chains=%lld k=%d..%d\n",S.size(),target,threads,chains,kmin,kmax);fflush(stdout);
    atomic<int> best((int)S.size());atomic<long long> done(0);atomic<bool> stop(false);
    vector<thread> th;
    auto t0=chrono::steady_clock::now();
    for(int t=0;t<threads;t++){
        th.emplace_back([&,t]{
            vector<uint16_t> cnt(N5,0);vector<char> in(N5,0);vector<int> cur;
            for(int v:S){if(cnt[v]==0){in[v]=1;cur.push_back(v);for(auto u:nb[v])cnt[u]++;}}
            Rng rng(0x9E3779B97F4A7C15ULL*(t+1)+0x1234567);
            vector<int> removed,added,cand;
            for(long long it=0;it<chains&&!stop.load(memory_order_relaxed);it++){
                int k=kmin+(int)rng.below((uint32_t)(kmax-kmin+1));
                if(k>(int)cur.size())k=(int)cur.size();
                removed.clear();added.clear();
                for(int i=0;i<k;i++){
                    int j=(int)rng.below((uint32_t)cur.size());
                    int v=cur[j];
                    removed.push_back(v);
                    cur[j]=cur.back();cur.pop_back();
                    in[v]=0;for(auto u:nb[v])cnt[u]--;
                }
                cand.clear();
                for(int v:removed){ if(!in[v]&&cnt[v]==0)cand.push_back(v); }
                for(int v:removed)for(auto u:nb[v])if(!in[u]&&cnt[u]==0)cand.push_back(u);
                sort(cand.begin(),cand.end());cand.erase(unique(cand.begin(),cand.end()),cand.end());
                for(size_t i=cand.size();i>1;i--){size_t j=rng.below((uint32_t)i);swap(cand[i-1],cand[j]);}
                for(int u:cand){
                    if(in[u]||cnt[u]!=0)continue;
                    in[u]=1;cur.push_back(u);added.push_back(u);
                    for(auto w:nb[u])cnt[w]++;
                }
                int sz=(int)cur.size();
                if(sz>=(int)S.size()){          // keep (monotone + plateau)
                    if(sz>(int)best.load())best.store(sz);
                }else{
                    for(int u:added){in[u]=0;for(auto w:nb[u])cnt[w]--;}
                    size_t w=0;
                    for(size_t i=0;i<cur.size();i++)if(in[cur[i]])cur[w++]=cur[i];
                    cur.resize(w);
                    for(int v:removed){in[v]=1;cur.push_back(v);for(auto u:nb[v])cnt[u]++;}
                }
                done.fetch_add(1,memory_order_relaxed);
                if((int)cur.size()>=target){
                    vector<int> keep=cur;
                    if(independent(keep)){
                        stop.store(true);
                        FILE*o=fopen("LNS6_FOUND.txt","w");
                        if(o){fprintf(o,"SIZE %zu\n",keep.size());
                              for(int v:keep)fprintf(o,"%d %d %d %d %d\n",dig[v][0],dig[v][1],dig[v][2],dig[v][3],dig[v][4]);
                              fclose(o);}
                        printf("FOUND size=%zu\n",keep.size());fflush(stdout);
                    }
                }
            }
        });
    }
    for(auto&x:th)x.join();
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("LNS6 best=%d chains=%lld seconds=%.1f rate=%.0f/s\n",best.load(),done.load(),sec,done.load()/sec);
    return best.load()>=target?0:2;
}
