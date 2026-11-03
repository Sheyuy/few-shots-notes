// lnsd: dimension-generic ruin-and-recreate LNS for independent sets in C7^D.
// D=5 used as regression (known exhaustive answer), D=10 for the real record hunt.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;
static int D;static long long U;static vector<long long> pw;
static vector<vector<signed char>> OFF;       // 3^D-1 offset vectors
static vector<uint16_t> cnt;
static vector<uint8_t> inS;
struct Rng{uint64_t s;explicit Rng(uint64_t x):s(x?x:88172645463325252ULL){}
    inline uint64_t next(){s^=s<<13;s^=s>>7;s^=s<<17;return s;}
    inline uint32_t below(uint32_t n){return (uint32_t)(next()%(uint64_t)n);}};
static inline long long neigh(int v,int mask){
    long long w=0,x=v;
    for(int i=0;i<D;i++){int d=(int)(x%7);x/=7;int o=OFF[mask][i];d+=o;if(d<0)d+=7;else if(d>6)d-=7;w+=pw[i]*d;}
    return w;
}
int main(int argc,char**argv){
    D=argc>1?atoi(argv[1]):5;
    const char* path=argc>2?argv[2]:"R367.txt";
    int kmin=argc>3?atoi(argv[3]):6, kmax=argc>4?atoi(argv[4]):14;
    long long iters=argc>5?atoll(argv[5]):100000;
    int target=argc>6?atoi(argv[6]):-1;
    U=1;pw.assign(D,1);for(int i=1;i<D;i++)pw[i]=pw[i-1]*7;U=1;for(int i=0;i<D;i++)U*=7;
    long long tot=1;for(int i=0;i<D;i++)tot*=3;
    OFF.clear();OFF.reserve(tot);
    for(long long m=0;m<tot;m++){
        long long t=m;vector<signed char> o(D);bool any=false;
        for(int i=0;i<D;i++){o[i]=(signed char)((t%3)-1);if(o[i])any=true;t/=3;}
        if(any)OFF.push_back(o);
    }
    long long nm=(long long)OFF.size();   // = 3^D - 1, self excluded
    cnt.assign(U,0);inS.assign(U,0);
    // read set: D digits, either packed string or space separated
    FILE*f=fopen(path,"r");if(!f){printf("NO_FILE %s\n",path);return 1;}
    vector<int> S;char line[256];
    while(fgets(line,sizeof line,f)){
        int d[16],got=0;
        for(char*p=line;*p&&got<D;p++){ if(*p>='0'&&*p<='6')d[got++]=*p-'0'; }
        if(got<D)continue;
        long long v=0;for(int i=0;i<D;i++)v+=pw[i]*d[i];
        S.push_back((int)v);
    }
    fclose(f);
    printf("D=%d U=%lld set=%zu k=%d..%d iters=%lld\n",D,U,S.size(),kmin,kmax,iters);fflush(stdout);
    auto t0=chrono::steady_clock::now();
    for(int v:S){inS[v]=1;for(long long m=1;m<=nm;m++)cnt[neigh(v,(int)(m-1))]++;}
    double tbuild=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("BUILD cnt in %.1f s\n",tbuild);fflush(stdout);
    int maxc=0;for(long long v=0;v<U;v++)if(cnt[v]>maxc)maxc=cnt[v];
    printf("MAXCOUNT %d\n",maxc);fflush(stdout);
    vector<int> free0;
    for(long long v=0;v<U;v++)if(!inS[v]&&cnt[v]==0)free0.push_back((int)v);
    printf("FREE0 %zu\n",free0.size());fflush(stdout);
    vector<int> cur=S;Rng rng(12345);
    vector<int> removed,added,cand;vector<uint32_t> stamp(U,0);uint32_t st=0;
    if(target<0)target=(int)cur.size();
    int best=(int)cur.size();
    auto t1=chrono::steady_clock::now();
    for(long long it=0;it<iters;it++){
        int oldsize=(int)cur.size();
        int k=kmin+(int)rng.below((uint32_t)(kmax-kmin+1));
        if(k>(int)cur.size())k=(int)cur.size();
        removed.clear();added.clear();
        for(int i=0;i<k;i++){
            int j=(int)rng.below((uint32_t)cur.size());
            int v=cur[j];removed.push_back(v);
            cur[j]=cur.back();cur.pop_back();
            inS[v]=0;for(long long m=1;m<=nm;m++)cnt[neigh(v,(int)(m-1))]--;
        }
        cand.clear();st++;
        for(int v:free0)if(!inS[v]&&cnt[v]==0){stamp[v]=st;cand.push_back(v);}
        for(int v:removed)for(long long m=1;m<=nm;m++){
            long long x=neigh(v,(int)(m-1));
            if(inS[x]||cnt[x])continue;
            if(stamp[x]==st)continue;
            stamp[x]=st;cand.push_back((int)x);
        }
        for(size_t i=cand.size();i>1;i--){size_t j=rng.below((uint32_t)i);swap(cand[i-1],cand[j]);}
        for(int u:cand){
            if(inS[u]||cnt[u])continue;
#ifdef DEBUG
            { int bad=-1;for(long long m=1;m<=nm;m++){long long x=neigh(u,(int)(m-1));if(inS[x]){bad=(int)x;break;}}
              if(bad>=0){
                  int nbInSet=0,nbInRemoved=0;
                  for(long long m=1;m<=nm;m++){long long x=neigh(u,(int)(m-1));if(inS[x])nbInSet++;
                      for(int r:removed)if((int)x==r){nbInRemoved++;break;}}
                  int cntIn=0;for(size_t z=0;z<inS.size();z++)cntIn+=inS[z];
                  int inCurNotIn=0;for(int v:cur)if(!inS[v])inCurNotIn++;
                  printf("ADD_BAD inS_total=%d cur=%zu inCurNotIn=%d oldsize=%d\n",cntIn,cur.size(),inCurNotIn,oldsize);
                  printf("ADD_BAD u=%d bad=%d cnt_u=%d size=%zu it=%lld nbInSet=%d nbInR=%d k=%d removed:",u,bad,(int)cnt[u],cur.size(),it,nbInSet,nbInRemoved,k);
                  for(int r:removed)printf(" %d",r);printf("\n");fflush(stdout);return 5;} }
#endif
            inS[u]=1;cur.push_back(u);added.push_back(u);
            for(long long m=1;m<=nm;m++)cnt[neigh(u,(int)(m-1))]++; }
        if(getenv("LNSD_VERBOSE")&&it<8)printf("IT %lld old=%d k=%d added=%zu size=%zu\n",it,oldsize,k,added.size(),cur.size());
        if((int)cur.size()>best){best=(int)cur.size();printf("BEST %d at it=%lld\n",best,it);fflush(stdout);}
        int reverted=0;
        if((int)cur.size()<oldsize){
            reverted=1;
            for(int u:added){inS[u]=0;for(long long m=1;m<=nm;m++)cnt[neigh(u,(int)(m-1))]--;}
            size_t w=0;for(size_t i=0;i<cur.size();i++)if(inS[cur[i]])cur[w++]=cur[i];
            cur.resize(w);
            for(int v:removed){inS[v]=1;cur.push_back(v);for(long long m=1;m<=nm;m++)cnt[neigh(v,(int)(m-1))]++;}
        }
#ifdef DEBUG
        {   vector<uint16_t> c2(inS.size(),0);
            for(int v:cur)for(long long m=1;m<=nm;m++)c2[neigh(v,(int)(m-1))]++;
            int badv=-1;for(size_t z=0;z<c2.size();z++)if(c2[z]!=cnt[z]){badv=(int)z;break;}
            if(badv>=0){printf("DESYNC_AFTER_IT it=%lld v=%d cnt=%d true=%d size=%zu old=%d k=%d removed=%zu added=%zu reverted=%d\n",
                    it,badv,(int)cnt[badv],(int)c2[badv],cur.size(),oldsize,k,removed.size(),added.size(),(int)reverted);
                fflush(stdout);return 7;}
        }
        {   int cntIn=0;for(size_t z=0;z<inS.size();z++)cntIn+=inS[z];
            int ghost=0;for(int v:cur)if(!inS[v])ghost++;
            if(cntIn!=(int)cur.size()||ghost){
                printf("INV_FAIL it=%lld cur=%zu inS=%d ghosts=%d removed=%zu added=%zu\n",it,cur.size(),cntIn,ghost,removed.size(),added.size());
                fflush(stdout);return 6;
            }
        }
        {   vector<char> t(inS.size(),0);
            bool dup=false;for(int v:cur){if(t[v]){dup=true;break;}t[v]=1;}
            bool conf=false;int bu=-1,bw=-1;
            for(int v:cur){for(long long m=1;m<=nm;m++){long long x=neigh(v,(int)(m-1));if(x!=v&&t[x]){conf=true;bu=v;bw=(int)x;break;}}if(conf)break;}
            if(dup||conf){
                vector<uint16_t> c2(inS.size(),0);
                for(int v:cur)for(long long m=1;m<=nm;m++){long long x=neigh(v,(int)(m-1));c2[x]++;}
                int badv=-1;for(size_t v=0;v<c2.size();v++)if(c2[v]!=cnt[v]){badv=(int)v;break;}
                printf("DBG_FAIL it=%lld dup=%d conf=%d u=%d w=%d size=%zu added=%zu removed=%zu mismatch_v=%d cnt=%d recomputed=%d\n",
                       it,(int)dup,(int)conf,bu,bw,cur.size(),added.size(),removed.size(),badv,
                       badv>=0?(int)cnt[badv]:-1, badv>=0?(int)c2[badv]:-1);
                fflush(stdout);return 4;}
        }
#endif
        if((int)cur.size()>=target){
            // verify: every member must have cnt==0
            bool ok=true;int first=-1;
            for(int v:cur)if(cnt[v]!=0){ok=false;first=v;break;}
            if(!ok){
                printf("FOUND_BAD v=%d cnt=%d nbInSet=",first,(int)cnt[first]);
                for(long long m=1;m<=nm;m++){long long x=neigh(first,(int)(m-1));if(inS[x])printf("%d ",(int)x);}
                printf(" size=%zu it=%lld\n",cur.size(),it);fflush(stdout);
            }
            if(ok){
                printf("FOUND size=%zu it=%lld\n",cur.size(),it);fflush(stdout);
                FILE*o=fopen("LNSD_FOUND.txt","w");
                for(int v:cur){long long x=v;for(int i=0;i<D;i++){fprintf(o,"%d",(int)(x%7));x/=7;}fprintf(o,"\n");}
                fclose(o);return 0;
            }else{printf("BAD_STATE size=%zu\n",cur.size());return 3;}
        }
    }
    double t2=chrono::duration<double>(chrono::steady_clock::now()-t1).count();
    printf("LNSD D=%d best=%d iters=%lld %.1f s %.0f it/s\n",D,best,iters,t2,iters/t2);
    return 2;
}
