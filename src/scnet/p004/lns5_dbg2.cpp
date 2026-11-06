// lns5: local search for a larger independent set in C7^5 (target 368).
// Ruin-and-recreate: drop k random points, greedily re-add compatible points.
// Heuristic only -- it can find a witness but never proves absence.
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
static inline void builddig(){for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}}
static inline void buildnb(){
    for(int v=0;v<N5;v++){
        int d[5];for(int i=0;i<5;i++)d[i]=dig[v][i];
        int idx[5];vector<uint16_t> t;
        for(idx[0]=-1;idx[0]<=1;idx[0]++)for(idx[1]=-1;idx[1]<=1;idx[1]++)
        for(idx[2]=-1;idx[2]<=1;idx[2]++)for(idx[3]=-1;idx[3]<=1;idx[3]++)
        for(idx[4]=-1;idx[4]<=1;idx[4]++){
            bool any=false;int x=0;
            for(int i=4;i>=0;i--){int c=(d[i]+idx[i]+7)%7;if(idx[i])any=true;x=x*7+c;}
            if(any)t.push_back((uint16_t)x);
        }
        nb[v]=t;
    }
}
struct Rng{uint64_t s;Rng(uint64_t x):s(x?x:88172645463325252ULL){}
    inline uint64_t next(){s^=s<<13;s^=s>>7;s^=s<<17;return s;}
    inline uint32_t below(uint32_t n){return (uint32_t)(next()%(uint64_t)n);}
};
int main(int argc,char**argv){
    int threads=argc>1?atoi(argv[1]):8;
    long long chains=argc>2?atoll(argv[2]):1000000;
    const char* path=argc>3?argv[3]:"R367.txt";
    builddig();buildnb();
    FILE*f=fopen(path,"r");if(!f){printf("NO_FILE %s\n",path);return 1;}
    vector<int> S;int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5){int x=0;for(int i=4;i>=0;i--)x=x*7+a[i];S.push_back(x);}
    fclose(f);
    printf("START size=%zu threads=%d chains=%lld\n",S.size(),threads,chains);fflush(stdout);
    atomic<int> best((int)S.size()); atomic<long long> done(0);
    atomic<bool> stop(false);
    vector<thread> th;
    auto t0=chrono::steady_clock::now();
    for(int t=0;t<threads;t++){
        th.emplace_back([&,t]{
            vector<uint16_t> cnt(N5,0); vector<char> in(N5,0);
            vector<int> cur;
            for(int v:S){if(cnt[v]==0){in[v]=1;cur.push_back(v);for(auto u:nb[v])cnt[u]++;}}
            Rng rng(0x9E3779B97F4A7C15ULL*(t+1)+12345);
            vector<int> removed, added, cand;
            for(long long it=0;it<chains&&!stop.load(memory_order_relaxed);it++){
                int k=8+rng.below(13);            // drop 8..20 points
                if(k>(int)cur.size())k=cur.size();
                removed.clear();added.clear();
                for(int i=0;i<k;i++){
                    int j=rng.below((uint32_t)cur.size());
                    int v=cur[j];
                    removed.push_back(v);
                    // lazy delete: swap-remove
                    cur[j]=cur.back();cur.pop_back();
                    in[v]=0;for(auto u:nb[v])cnt[u]--;
                }
                cand.clear();
                for(int v:removed)for(auto u:nb[v])if(!in[u]&&cnt[u]==0)cand.push_back(u);
                sort(cand.begin(),cand.end());cand.erase(unique(cand.begin(),cand.end()),cand.end());
                for(size_t i=cand.size();i>1;i--){size_t j=rng.below((uint32_t)i);swap(cand[i-1],cand[j]);}
                for(int u:cand){
                    if(in[u]||cnt[u]!=0)continue;
                    {   int bad=-1;
                        for(auto w:nb[u])if(in[w]){bad=w;break;}
                        if(bad>=0){printf("ADD_CONFLICT u=%d w=%d cnt_u=%d size=%zu\n",u,bad,(int)cnt[u],cur.size());fflush(stdout);stop.store(true);return;}
                    }
                    in[u]=1;cur.push_back(u);added.push_back(u);
                    for(auto w:nb[u])cnt[w]++;
                }
                int sz=(int)cur.size();
                if(sz>(int)S.size()||(sz==(int)S.size()&&rng.below(2))){
                    // keep (plateau walk)
                }else{
                    for(int u:added){in[u]=0;for(auto w:nb[u])cnt[w]--;}
                    for(int v:removed){in[v]=1;cur.push_back(v);for(auto u:nb[v])cnt[u]++;}
                }
                int now=(int)cur.size();
                if(now>(int)best.load()){best.store(now);}
                done.fetch_add(1,memory_order_relaxed);
                if(now>=368){
                    bool ok=true;
                    for(size_t i=0;i<cur.size()&&ok;i++)for(size_t j=i+1;j<cur.size();j++)
                        if(cnt[cur[i]]&&false){}
                    for(size_t i=0;i<cur.size()&&ok;i++)for(auto u:nb[cur[i]])if(in[u]&&u!=cur[i]){ok=false;break;}
                    if(!ok){printf("BAD_WITNESS_INTERNAL\n");continue;}
                    stop.store(true);
                    FILE*o=fopen("LNS5_FOUND.txt","a");
                    if(o){fprintf(o,"SIZE %d\n",now);for(int v:cur)fprintf(o,"%d %d %d %d %d\n",dig[v][0],dig[v][1],dig[v][2],dig[v][3],dig[v][4]);fclose(o);}
                }
            }
        });
    }
    for(auto&x:th)x.join();
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("LNS5 best=%d chains=%lld seconds=%.1f rate=%.0f/s\n",best.load(),done.load(),sec,done.load()/sec);
    return best.load()>=368?0:2;
}
