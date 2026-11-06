// C++17 audit rewrite of the existing p004 scratch program.
// Compile explicitly: g++ -x c++ -std=c++17 -O3 -fopenmp sixm3.c -o sixm3
// No files are written by this program. A bounded run emits witnesses or status.
// Completeness through k<=5: a minimal improving exchange has connected blocker
// hypergraph, else a component is a smaller improvement. Enumerate natural rank-k
// edges; rank-(k-1) edges plus one connected extension; rank-3 edges plus one/two
// connected extensions for k=5; and ALL connected pair-graph sets (not just paths
// and stars). Records/pools are dynamically sized. Timeouts are INCOMPLETE.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
#include <omp.h>
using U=uint32_t; using Q=uint64_t;
constexpr U HALF=16807, WORDS=263, SPACE=282475249, NIL=UINT32_MAX;
struct Key {
    std::array<U,6> v{}; uint8_t n=0;
    bool operator==(const Key& b)const{return n==b.n&&v==b.v;}
    bool operator<(const Key& b)const{return n!=b.n?n<b.n:v<b.v;}
};
Q mix(Q x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
struct Hash{size_t operator()(const Key& a)const{Q h=mix(a.n);for(int i=0;i<a.n;i++)h=mix(h^a.v[i]);return h;}};
struct ConstantHash{size_t operator()(const Key&)const{return 0;}};
struct Rec{Key b; U point=NIL,next=NIL;};
struct Counters{Q sets=0,eligible=0;U maxpool=0;};
std::vector<U> baseline;
std::unordered_map<U,U> dense;
std::vector<Q> nbmask,members;
std::vector<std::array<U,243>> neighbours;
std::vector<std::vector<U>> nonemptyNeighbours;
std::vector<Rec> records;
std::unordered_map<Key,U,Hash> table;
std::array<std::vector<U>,7> heads;
std::array<std::vector<std::vector<U>>,7> incidence;
std::vector<Key> connectedPairs;
std::atomic<bool> expired{false},found{false};
std::atomic<Q> blockerErrors{0};
std::chrono::steady_clock::time_point deadline;
Key witnessD; std::vector<U> witnessA;
int maxK=5,threads=16,secondsBudget=240;
int shardIndex=0,shardCount=1;bool deep6=false,onlyDeep6=false,deep3=false,onlyDeep3=false,conn6=false,onlyConn6=false;int probeR3=0;
inline bool mine(size_t i){return shardCount==1||(i%(size_t)shardCount)==(size_t)shardIndex;}

bool directConflict(U x,U y){
    for(int i=0;i<10;i++){int a=x%7,b=y%7,d=std::abs(a-b);if(d!=0&&d!=1&&d!=6)return false;x/=7;y/=7;}return true;
}
bool maskConflict(U x,U y){
    U p=x/HALF,q=y/HALF,s=x%HALF,t=y%HALF;
    return ((nbmask[size_t(p)*WORDS+q/64]>>(q%64))&1)&&((nbmask[size_t(s)*WORDS+t/64]>>(t%64))&1);
}
bool graphDFS(Q available,int need,const std::vector<Q>& conflict,std::vector<int>& chosen){
    if(need==0)return true;
    while(__builtin_popcountll(available)>=need){
        int v=__builtin_ctzll(available);available&=available-1;chosen.push_back(v);
        if(graphDFS(available&~conflict[v],need-1,conflict,chosen))return true;
        chosen.pop_back();
    }return false;
}
bool largeDFS(const std::vector<int>& avail,int need,const std::vector<U>& pool,std::vector<int>& chosen){
    if(need==0)return true;
    for(size_t i=0;i+size_t(need)<=avail.size();i++){
        int v=avail[i];std::vector<int> next;
        for(size_t j=i+1;j<avail.size();j++)if(!maskConflict(pool[v],pool[avail[j]]))next.push_back(avail[j]);
        chosen.push_back(v);if(largeDFS(next,need-1,pool,chosen))return true;chosen.pop_back();
    }return false;
}
std::vector<int> independent(const std::vector<U>& pool,int need){
    std::vector<int> chosen;
    if(pool.size()<size_t(need))return chosen;
    if(pool.size()<=64){
        std::vector<Q> conflict(pool.size());
        for(size_t i=0;i<pool.size();i++)for(size_t j=i+1;j<pool.size();j++)if(maskConflict(pool[i],pool[j])){
            conflict[i]|=Q(1)<<j;conflict[j]|=Q(1)<<i;
        }
        Q all=pool.size()==64?~Q(0):(Q(1)<<pool.size())-1;
        graphDFS(all,need,conflict,chosen);
    }else{
        std::vector<int> avail(pool.size());std::iota(avail.begin(),avail.end(),0);largeDFS(avail,need,pool,chosen);
    }
    return chosen;
}
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void selftest(){
    for(U a=0;a<7;a++)for(U b=0;b<7;b++){
        int delta=(int(a)-int(b)+7)%7;bool expect=delta==0||delta==1||delta==6;
        require(directConflict(a,b)==expect,"coordinate adjacency mismatch");
        require(directConflict(a,b)==directConflict(b,a),"asymmetric adjacency");
    }
    std::mt19937 rng(20261030);
    for(int trial=0;trial<1000;trial++){
        int n=2+rng()%11;std::vector<Q> g(n);
        for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)if(rng()%2){g[i]|=Q(1)<<j;g[j]|=Q(1)<<i;}
        int exact=0;
        for(unsigned m=0;m<(1u<<n);m++){
            bool ok=true;for(int i=0;i<n;i++)if((m>>i)&1u)if(g[i]&m){ok=false;break;}
            if(ok)exact=std::max(exact,__builtin_popcount(m));
        }
        for(int need=1;need<=6;need++){
            std::vector<int> selected;bool ok=graphDFS((Q(1)<<n)-1,need,g,selected);
            require(ok==(exact>=need),"independent-set search disagrees with brute force");
            if(ok)require(selected.size()==size_t(need),"witness cardinality mismatch");
        }
    }
    std::unordered_map<Key,int,ConstantHash> collision;
    for(int i=0;i<100;i++){Key k;k.n=5;for(int j=0;j<5;j++)k.v[j]=U(i*100003+j*268435457u);collision.emplace(k,i);}
    require(collision.size()==100,"full key equality failed under forced collisions");
    std::cout<<"SELFTEST coordinate_cases=49 random_graphs=1000 targets_per_graph=6 forced_hash_collisions=100 PASS\n"<<std::flush;
}
void load(const std::string& path){
    std::ifstream f(path);require(bool(f),"cannot open baseline input");std::string s;
    while(f>>s){require(s.size()==10,"invalid baseline point length");U x=0;for(char c:s){require(c>='0'&&c<='6',"invalid baseline digit");x=x*7+U(c-'0');}baseline.push_back(x);}
    require(!f.bad(),"baseline read failed");std::sort(baseline.begin(),baseline.end());
    require(std::adjacent_find(baseline.begin(),baseline.end())==baseline.end(),"duplicate baseline point");
    require(baseline.size()==134753,"unexpected baseline size");
    dense.reserve(baseline.size()*2);for(U i=0;i<baseline.size();i++)dense.emplace(baseline[i],i);
    nbmask.assign(size_t(HALF)*WORDS,0);members.assign(size_t(HALF)*WORDS,0);neighbours.resize(HALF);
    #pragma omp parallel for schedule(static)
    for(U x=0;x<HALF;x++){
        U z=x;std::array<U,5>d{};for(int k=4;k>=0;k--){d[k]=z%7;z/=7;}
        for(U m=0;m<243;m++){
            U a=m,y=0;for(int j=0;j<5;j++){U o=a%3;a/=3;y=y*7+(d[j]+6+o)%7;}
            neighbours[x][m]=y;nbmask[size_t(x)*WORDS+y/64]|=Q(1)<<(y%64);
        }
    }
    std::vector<int> groupSize(HALF);
    for(U x:baseline){U p=x/HALF,s=x%HALF;members[size_t(p)*WORDS+s/64]|=Q(1)<<(s%64);groupSize[p]++;}
    nonemptyNeighbours.resize(HALF);
    for(U p=0;p<HALF;p++)for(U q:neighbours[p])if(groupSize[q])nonemptyNeighbours[p].push_back(q);
    std::mt19937 rng(91731);
    for(int i=0;i<100000;i++){
        U x=rng()%SPACE,y=rng()%SPACE;
        require(maskConflict(x,y)==directConflict(x,y),"neighbour mask differs from digit predicate");
        require(maskConflict(x,y)==maskConflict(y,x),"neighbour mask is asymmetric");
    }
    std::cout<<"MASK_SELFTEST random_pairs=100000 PASS baseline="<<baseline.size()<<"\n"<<std::flush;
}
Key actualBlockers(U x){
    Key k;U p=x/HALF,s=x%HALF;const Q* nm=&nbmask[size_t(s)*WORDS];
    for(U q:nonemptyNeighbours[p]){
        const Q* mp=&members[size_t(q)*WORDS];
        for(U w=0;w<WORDS;w++){
            Q bits=mp[w]&nm[w];while(bits){U bit=__builtin_ctzll(bits);bits&=bits-1;
                if(k.n>=6){k.n=7;return k;}
                U point=q*HALF+w*64+bit;
                auto it=dense.find(point);if(it==dense.end()){k.n=6;return k;}
                k.v[k.n++]=it->second;
            }
        }
    }
    std::sort(k.v.begin(),k.v.begin()+k.n);return k;
}
void collect(){
    std::vector<uint16_t>a(SPACE),b(SPACE);for(U x:baseline)a[x]=1;
    U stride=1;
    for(int dim=0;dim<10;dim++,stride*=7){
        #pragma omp parallel for schedule(static)
        for(U g=0;g<SPACE/7;g++){
            size_t start=size_t(g/stride)*7*stride+g%stride;
            uint16_t v[7];for(int d=0;d<7;d++)v[d]=a[start+size_t(d)*stride];
            for(int d=0;d<7;d++)b[start+size_t(d)*stride]=uint16_t(v[(d+6)%7]+v[d]+v[(d+1)%7]);
        }a.swap(b);
    }
    b.clear();b.shrink_to_fit();std::array<Q,7>expected{};
    for(U x=0;x<SPACE;x++)if(a[x]<=6)expected[a[x]]++;
    if(expected[0]>0){
        for(U x=0;x<SPACE;x++)if(a[x]==0){witnessD=Key{};witnessA={x};found=true;break;}
        std::cout<<"ZERO_BLOCKERS count="<<expected[0]<<" candidate_addition=1\n"<<std::flush;
        return;
    }
    require(expected[1]>=baseline.size(),"invalid closed-neighbour count");expected[1]-=baseline.size();
    Q total=0;for(int r=1;r<=6;r++)total+=expected[r];records.reserve(total);
    std::vector<std::vector<Rec>> local(omp_get_max_threads());
    #pragma omp parallel
    {
        auto& out=local[omp_get_thread_num()];out.reserve(size_t(total/omp_get_num_threads()+10000));
        #pragma omp for schedule(dynamic,8)
        for(U p=0;p<HALF;p++)for(U s=0;s<HALF;s++){
            U x=p*HALF+s;int m=a[x];if(m<1||m>6)continue;
            if((members[size_t(p)*WORDS+s/64]>>(s%64))&1)continue;
            Key k=actualBlockers(x);if(k.n!=m){blockerErrors++;continue;}
            out.push_back(Rec{k,x,NIL});
        }
    }
    require(blockerErrors==0,"recovered blockers disagree with convolution");
    for(auto& l:local){records.insert(records.end(),l.begin(),l.end());std::vector<Rec>().swap(l);}
    std::array<Q,7>actual{};for(const auto& r:records)actual[r.b.n]++;
    for(int r=1;r<=6;r++)require(expected[r]==actual[r],"record-count mismatch or truncation");
    require(total==records.size(),"lost records");
    std::cout<<"COUNTS m0=0";for(int r=1;r<=6;r++)std::cout<<" m"<<r<<"="<<actual[r];std::cout<<" blocker_mismatches=0 truncations=0\n"<<std::flush;
    a.clear();a.shrink_to_fit();table.max_load_factor(.75f);table.reserve(records.size());
    for(U i=0;i<records.size();i++){
        auto p=table.emplace(records[i].b,i);
        if(p.second)heads[records[i].b.n].push_back(i);
        else{records[i].next=p.first->second;p.first->second=i;}
    }
    Q linked=0;U maxMultiplicity=0;
    for(const auto& p:table){U n=0;for(U i=p.second;i!=NIL;i=records[i].next){n++;linked++;}maxMultiplicity=std::max(maxMultiplicity,n);}
    require(linked==records.size(),"linked table lost or duplicated records");
    for(int r=2;r<=5;r++){
        incidence[r].resize(baseline.size());
        for(U i:heads[r])for(int j=0;j<r;j++)incidence[r][records[i].b.v[j]].push_back(i);
    }
    for(U i:heads[2])connectedPairs.push_back(records[i].b);
    std::sort(connectedPairs.begin(),connectedPairs.end());
    std::cout<<"KEYS";for(int r=1;r<=6;r++)std::cout<<" rank"<<r<<"="<<heads[r].size();std::cout<<" max_multiplicity="<<maxMultiplicity<<" linked_records="<<linked<<"\n"<<std::flush;
}
std::vector<U> poolOf(const Key& d){
    std::vector<U> pool;
    for(unsigned m=1;m<(1u<<d.n);m++){
        Key k;for(int j=0;j<d.n;j++)if(m&(1u<<j))k.v[k.n++]=d.v[j];
        auto p=table.find(k);if(p==table.end())continue;
        for(U i=p->second;i!=NIL;i=records[i].next)pool.push_back(records[i].point);
    }std::sort(pool.begin(),pool.end());
    if(std::adjacent_find(pool.begin(),pool.end())!=pool.end())throw std::runtime_error("duplicate pool point");
    return pool;
}
bool subset(const Key& a,const Key& b){for(int i=0;i<a.n;i++)if(!std::binary_search(b.v.begin(),b.v.begin()+b.n,a.v[i]))return false;return true;}
bool stopped(){
    if(found.load(std::memory_order_relaxed)||expired.load(std::memory_order_relaxed))return true;
    if(std::chrono::steady_clock::now()>deadline){expired=true;return true;}return false;
}
void check(const Key& d,Counters& c){
    if(found.load(std::memory_order_relaxed))return;
    auto pool=poolOf(d);c.sets++;c.maxpool=std::max(c.maxpool,U(pool.size()));
    if(pool.size()<=d.n)return;c.eligible++;
    auto chosen=independent(pool,int(d.n)+1);if(chosen.empty())return;
    std::vector<U> add;for(int i:chosen)add.push_back(pool[i]);
    require(add.size()==size_t(d.n+1),"bad candidate cardinality");
    for(size_t i=0;i<add.size();i++){
        require(dense.find(add[i])==dense.end(),"candidate is already in baseline");
        Key bl=actualBlockers(add[i]);require(bl.n<=6&&subset(bl,d),"candidate has undeleted blocker");
        for(size_t j=0;j<i;j++)require(!directConflict(add[i],add[j]),"candidate fails independent predicate");
    }
    #pragma omp critical(witness)
    {if(!found){witnessD=d;witnessA=add;found=true;}}
}
void report(const std::string& name,const std::vector<Counters>& stats,bool complete){
    Counters s;for(auto c:stats){s.sets+=c.sets;s.eligible+=c.eligible;s.maxpool=std::max(s.maxpool,c.maxpool);}
    std::cout<<"STAGE "<<name<<" sets_or_anchor_instances="<<s.sets<<" eligible="<<s.eligible<<" max_pool="<<s.maxpool<<" complete="<<complete<<" found="<<found.load();if(shardCount>1)std::cout<<" shard="<<shardIndex<<"/"<<shardCount;std::cout<<"\n"<<std::flush;
}
void sample(const Key& d,const std::string& label){
    auto p=poolOf(d);std::cout<<"SAMPLE {\"label\":\""<<label<<"\",\"removed\":[";
    for(int j=0;j<d.n;j++){if(j)std::cout<<',';std::cout<<baseline[d.v[j]];}
    std::cout<<"],\"pool\":[";for(size_t j=0;j<p.size();j++){if(j)std::cout<<',';std::cout<<p[j];}std::cout<<"]}\n"<<std::flush;
}
Key merge(const Key& a,const Key& b,int limit){
    Key out;int i=0,j=0;
    while(i<a.n||j<b.n){
        U v;
        if(j==b.n||(i<a.n&&a.v[i]<b.v[j]))v=a.v[i++];
        else if(i==a.n||b.v[j]<a.v[i])v=b.v[j++];
        else{v=a.v[i++];j++;}
        if(out.n>=limit)return Key{};out.v[out.n++]=v;
    }if(out.n==a.n)return Key{};return out;
}
std::vector<Key> extensions(const Key& a,int rank,int limit){
    std::vector<Key> out;
    for(int r=2;r<=rank;r++)for(int j=0;j<a.n;j++)for(U idx:incidence[r][a.v[j]]){
        Key d=merge(a,records[idx].b,limit);if(d.n>a.n)out.push_back(d);
    }
    std::sort(out.begin(),out.end());out.erase(std::unique(out.begin(),out.end()),out.end());return out;
}
void naturalStage(int k){
    std::vector<Counters> stats(omp_get_max_threads());const auto& h=heads[k];
    #pragma omp parallel for schedule(dynamic,128)
    for(size_t i=0;i<h.size();i++){if(!mine(i))continue;if(!stopped())check(records[h[i]].b,stats[omp_get_thread_num()]);}
    report("natural_k"+std::to_string(k),stats,!expired&&!found);
    if(!h.empty())sample(records[h[h.size()/2]].b,"natural_k"+std::to_string(k));
}
void anchorStage(int rank,int k){
    std::vector<Counters> stats(omp_get_max_threads());const auto& h=heads[rank];
    #pragma omp parallel for schedule(dynamic,32)
    for(size_t i=0;i<h.size();i++){
        if(!mine(i))continue;
        if(stopped())continue;auto first=extensions(records[h[i]].b,rank,k);
        for(const Key& d:first){
            if(found||expired)break;
            if(d.n==k)check(d,stats[omp_get_thread_num()]);
            else{
                require(d.n+1==k,"unsupported extension depth");
                for(const Key& e:extensions(d,rank,k))if(e.n==k)check(e,stats[omp_get_thread_num()]);
            }
        }
    }
    report("rank"+std::to_string(rank)+"_extensions_k"+std::to_string(k),stats,!expired&&!found);
}
void pairStage(int k){
    std::vector<std::vector<Key>> local(omp_get_max_threads());
    #pragma omp parallel for schedule(dynamic,64)
    for(size_t i=0;i<connectedPairs.size();i++){
        if(stopped())continue;auto out=extensions(connectedPairs[i],2,k);
        auto& v=local[omp_get_thread_num()];v.insert(v.end(),out.begin(),out.end());
    }
    if(expired||found)return;
    size_t total=0;for(const auto& v:local)total+=v.size();std::vector<Key> next;next.reserve(total);
    for(auto& v:local){next.insert(next.end(),v.begin(),v.end());std::vector<Key>().swap(v);}
    std::sort(next.begin(),next.end());next.erase(std::unique(next.begin(),next.end()),next.end());
    connectedPairs.swap(next);std::cout<<"CONNECTED_PAIR_SETS k="<<k<<" generated="<<total<<" unique="<<connectedPairs.size()<<" truncated=0\n"<<std::flush;
    std::vector<Counters> stats(omp_get_max_threads());
    #pragma omp parallel for schedule(dynamic,128)
    for(size_t i=0;i<connectedPairs.size();i++)if(!stopped())check(connectedPairs[i],stats[omp_get_thread_num()]);
    report("connected_pair_k"+std::to_string(k),stats,!expired&&!found);
    if(!connectedPairs.empty())sample(connectedPairs[connectedPairs.size()/2],"connected_pair_k"+std::to_string(k));
}

bool hasBlocker4(const Key& s){
    int n=s.n;if(n<4)return false;
    for(int a=0;a<n;a++)for(int b=a+1;b<n;b++)for(int c=b+1;c<n;c++)for(int d=c+1;d<n;d++){
        Key q;q.n=4;q.v[0]=s.v[a];q.v[1]=s.v[b];q.v[2]=s.v[c];q.v[3]=s.v[d];
        if(table.find(q)!=table.end())return true;
    }
    return false;
}
void probeRank3(size_t sample){
    const auto& h3=heads[3];size_t step=std::max<size_t>(1,h3.size()/sample);
    Q n4=0,s4=0,n5=0,s5=0,n6=0,s6=0;size_t anchors=0;
    auto t0=std::chrono::steady_clock::now();
    for(size_t i=0;i<h3.size()&&anchors<sample;i+=step,anchors++){
        const Key& a=records[h3[i]].b;
        auto l4=extensions(a,3,4);n4+=l4.size();
        for(const Key& e:l4){
            if(hasBlocker4(e))continue;s4++;
            auto l5=extensions(e,4,5);n5+=l5.size();
            for(const Key& f:l5){
                if(hasBlocker4(f))continue;s5++;
                auto l6=extensions(f,4,6);n6+=l6.size();
                for(const Key& g:l6){if(!hasBlocker4(g))s6++;}
            }
        }
    }
    double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-t0).count();
    std::cout<<"PROBE_R3 anchors="<<anchors<<" step="<<step<<" heads3="<<h3.size()
             <<" four="<<n4<<" four_survive="<<s4<<" five="<<n5<<" five_survive="<<s5
             <<" six="<<n6<<" six_survive="<<s6<<" ms="<<(long long)ms<<"\n"<<std::flush;
}

void rank3Stage(int k){
    std::vector<Counters> stats(omp_get_max_threads());
    const auto& h=heads[3];
    #pragma omp parallel for schedule(dynamic,4)
    for(size_t i=0;i<h.size();i++){
        if(!mine(i))continue;
        if(stopped())continue;
        const Key& a=records[h[i]].b;
        auto l4=extensions(a,3,4);
        for(const Key& e:l4){
            if(found.load()||expired)break;
            if(hasBlocker4(e))continue;
            auto l5=extensions(e,4,5);
            for(const Key& f:l5){
                if(found.load()||expired)break;
                if(hasBlocker4(f))continue;
                auto l6=extensions(f,5,6);
                for(const Key& g:l6){
                    if(found.load()||expired)break;
                    if(hasBlocker4(g))continue;
                    check(g,stats[omp_get_thread_num()]);
                }
            }
        }
    }
    report("rank3_extensions_k6",stats,!expired&&!found);
}
int main(int argc,char** argv){
    try{
        std::string path="/tmp/p004_points.txt";bool selfOnly=false;
        for(int i=1;i<argc;i++){
            std::string arg=argv[i];
            if(arg=="--self-test")selfOnly=true;
            else if(arg=="--input"&&i+1<argc)path=argv[++i];
            else if(arg=="--max-k"&&i+1<argc)maxK=std::stoi(argv[++i]);
            else if(arg=="--threads"&&i+1<argc)threads=std::stoi(argv[++i]);
            else if(arg=="--seconds"&&i+1<argc)secondsBudget=std::stoi(argv[++i]);
            else if(arg=="--deep6")deep6=true;
            else if(arg=="--only-deep6"){deep6=true;onlyDeep6=true;}
            else if(arg=="--deep3")deep3=true;
            else if(arg=="--only-deep3"){deep3=true;onlyDeep3=true;}
            else if(arg=="--conn6")conn6=true;
            else if(arg=="--only-conn6"){conn6=true;onlyConn6=true;}
            else if(arg=="--probe-rank3"&&i+1<argc)probeR3=std::stoi(argv[++i]);
            else if(arg=="--shard"&&i+1<argc){std::string v=argv[++i];auto pos=v.find('/');require(pos!=std::string::npos,"--shard wants i/n");shardIndex=std::stoi(v.substr(0,pos));shardCount=std::stoi(v.substr(pos+1));require(shardCount>=1&&shardIndex>=0&&shardIndex<shardCount,"bad shard");}
            else throw std::runtime_error("invalid command line");
        }
        require(maxK>=1&&maxK<=6,"max-k must be in [1,5]");require(threads>=1&&threads<=64,"threads must be in [1,64]");
        require(secondsBudget>0&&secondsBudget<=86400,"seconds must be positive");
        omp_set_dynamic(0);omp_set_num_threads(threads);selftest();if(selfOnly)return 0;
        std::cout<<"SHARD {\"index\":"<<shardIndex<<",\"count\":"<<shardCount<<",\"deep6\":"<<(deep6?1:0)<<"}\n"<<std::flush;
        load(path);collect();deadline=std::chrono::steady_clock::now()+std::chrono::seconds(secondsBudget);
        if(probeR3>0){probeRank3(size_t(probeR3));return 0;}
        int completed=0;
        if(onlyConn6){
            require(maxK==6,"--only-conn6 requires --max-k 6");
            pairStage(3);if(!stopped())pairStage(4);if(!stopped())pairStage(5);if(!stopped())pairStage(6);
            if(!expired&&!found){
                if(shardCount==1){completed=6;std::cout<<"COMPLETE exchanges_up_to=6 no_improvement=1\n"<<std::flush;}
                else std::cout<<"SHARD_STAGE_DONE k=6 shard="<<shardIndex<<"/"<<shardCount<<"\n"<<std::flush;
            }
        }else if(onlyDeep3){
            require(maxK==6,"--only-deep3 requires --max-k 6");
            rank3Stage(6);
            if(!expired&&!found){
                if(shardCount==1){completed=6;std::cout<<"COMPLETE exchanges_up_to=6 no_improvement=1\n"<<std::flush;}
                else std::cout<<"SHARD_STAGE_DONE k=6 shard="<<shardIndex<<"/"<<shardCount<<"\n"<<std::flush;
            }
        }else if(onlyDeep6){
            require(maxK==6,"--only-deep6 requires --max-k 6");
            anchorStage(4,6);
            if(!stopped())std::cout<<"SKIP rank3_extensions_k6 reason=extension_depth_3_not_implemented\n"<<std::flush;
            if(!expired&&!found){
                if(shardCount==1){completed=6;std::cout<<"COMPLETE exchanges_up_to=6 no_improvement=1\n"<<std::flush;}
                else std::cout<<"SHARD_STAGE_DONE k=6 shard="<<shardIndex<<"/"<<shardCount<<"\n"<<std::flush;
            }
        }else
        for(int k=1;k<=maxK;k++){
            naturalStage(k);if(stopped())break;
            if(k==6){
                anchorStage(5,6);if(stopped())break;
                if(!deep6)break;
                anchorStage(4,6);if(stopped())break;
                if(deep3)rank3Stage(6);
                else std::cout<<"SKIP rank3_extensions_k6 reason=depth3_disabled(use --deep3)\n"<<std::flush;
                if(conn6)pairStage(6);
                if(shardCount==1){completed=k;std::cout<<"COMPLETE exchanges_up_to="<<k<<" no_improvement=1\n"<<std::flush;}
                else std::cout<<"SHARD_STAGE_DONE k=6 shard="<<shardIndex<<"/"<<shardCount<<"\n"<<std::flush;
                break;
            }
            for(int r=k-1;r>=3;r--){anchorStage(r,k);if(stopped())break;}
            if(stopped())break;
            if(k>=3)pairStage(k);
            if(stopped())break;completed=k;
            std::cout<<"COMPLETE exchanges_up_to="<<k<<" no_improvement=1\n"<<std::flush;
        }
        if(found){
            std::cout<<"RESULT {\"status\":\"candidate\",\"removed\":[";
            for(int j=0;j<witnessD.n;j++){if(j)std::cout<<',';std::cout<<baseline[witnessD.v[j]];}
            std::cout<<"],\"added\":[";for(size_t j=0;j<witnessA.size();j++){if(j)std::cout<<',';std::cout<<witnessA[j];}
            std::cout<<"],\"requires_official_verifier\":true}\n";
        }else std::cout<<"RESULT {\"status\":\""<<(expired?"incomplete_time_budget":"completed")<<"\",\"max_k_completed\":"<<completed<<",\"requested_max_k\":"<<maxK<<",\"found\":0,\"truncations\":0}\n";
        return expired?2:0;
    }catch(const std::exception& e){std::cerr<<"INCOMPLETE ERROR "<<e.what()<<'\n';return 3;}
}
