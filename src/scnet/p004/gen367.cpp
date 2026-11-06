// gen367: sample many distinct 367-point independent sets in C7^5 by
// large-kick ruin-and-recreate around a starting set (R367).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <set>
#include <string>
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
int main(int argc,char**argv){
    const char* start=argc>1?argv[1]:"R367.txt";
    int want=argc>2?atoi(argv[2]):30;      // how many distinct sets
    int kmin=argc>3?atoi(argv[3]):40, kmax=argc>4?atoi(argv[4]):120;
    long long tries=argc>5?atoll(argv[5]):2000000;
    builddig();buildnb();
    FILE*f=fopen(start,"r");if(!f){printf("NO_FILE %s\n",start);return 1;}
    vector<int> S;int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5){int x=0;for(int i=4;i>=0;i--)x=x*7+a[i];S.push_back(x);}
    fclose(f);
    int target=(int)S.size();
    printf("START %d target=%d want=%d k=%d..%d\n",target,target,want,kmin,kmax);
    Rng rng(0xDEADBEEF12345678ULL);
    vector<uint16_t> cnt(N5,0);vector<char> in(N5,0);vector<int> cur;
    for(int v:S){if(cnt[v]==0){in[v]=1;cur.push_back(v);for(auto u:nb[v])cnt[u]++;}}
    vector<int> removed,added,cand;
    set<string> seen; int found=0;
    string key; key.resize(target*2);
    for(long long it=0;it<tries&&found<want;it++){
        int k=kmin+(int)rng.below((uint32_t)(kmax-kmin+1));
        if(k>(int)cur.size())k=(int)cur.size();
        removed.clear();added.clear();
        for(int i=0;i<k;i++){
            int j=(int)rng.below((uint32_t)cur.size());
            int v=cur[j];removed.push_back(v);
            cur[j]=cur.back();cur.pop_back();
            in[v]=0;for(auto u:nb[v])cnt[u]--;
        }
        cand.clear();
        for(int v:removed)if(!in[v]&&cnt[v]==0)cand.push_back(v);
        for(int v:removed)for(auto u:nb[v])if(!in[u]&&cnt[u]==0)cand.push_back(u);
        sort(cand.begin(),cand.end());cand.erase(unique(cand.begin(),cand.end()),cand.end());
        for(size_t i=cand.size();i>1;i--){size_t j=rng.below((uint32_t)i);swap(cand[i-1],cand[j]);}
        for(int u:cand){
            if(in[u]||cnt[u]!=0)continue;
            in[u]=1;cur.push_back(u);added.push_back(u);
            for(auto w:nb[u])cnt[w]++;
        }
        if((int)cur.size()>=target){
            vector<int> s=cur;sort(s.begin(),s.end());
            // canonical key
            size_t p=0;
            for(int v:s){key[p++]=(char)('a'+dig[v][0]);key[p++]=(char)('a'+dig[v][1]);}
            if(seen.insert(key).second){
                char name[64];snprintf(name,sizeof name,"alt367_%02d.txt",found);
                FILE*o=fopen(name,"w");
                for(int v:s)fprintf(o,"%d %d %d %d %d\n",dig[v][0],dig[v][1],dig[v][2],dig[v][3],dig[v][4]);
                fclose(o);
                printf("SAVED %s size=%zu it=%lld\n",name,s.size(),it);fflush(stdout);
                found++;
            }
        }else{
            for(int u:added){in[u]=0;for(auto w:nb[u])cnt[w]--;}
            size_t w=0;for(size_t i=0;i<cur.size();i++)if(in[cur[i]])cur[w++]=cur[i];
            cur.resize(w);
            for(int v:removed){in[v]=1;cur.push_back(v);for(auto u:nb[v])cnt[u]++;}
        }
    }
    printf("DONE distinct=%d tries=%lld\n",found,tries);
    return 0;
}
