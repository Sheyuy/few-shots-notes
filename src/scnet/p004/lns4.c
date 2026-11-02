// Audited plateau-search extension, C++17.
// Build by concatenating sixm3.c (main renamed) and this source in one translation unit.
// The original core remains unchanged. This heuristic is NOT an exhaustive search.
// Every accepted move preserves independence; all candidate pools are complete for
// the selected removal set. No files are written; output is JSON deltas from baseline.
#include <deque>
#include <set>
#include <mutex>
namespace plateau {
std::vector<uint16_t> initialCounts;
std::vector<std::pair<U,U>> initialPrivate;
std::vector<U> initialLow;
std::vector<int> initialGroupSize;
std::chrono::steady_clock::time_point endTime;
std::atomic<bool> stopSearch{false};
std::atomic<int> bestRadius{0};
std::mutex outputMutex;
int chains=8,duration=120,radiusLimit=512;
struct Snapshot{std::vector<U> removed,added;int chain=-1;};
Snapshot bestEqual,breakthrough;
struct Stats {Q moves=0,singleMoves=0,blockMoves=0,blockTrials=0,exactPools=0,unknownPools=0,consistencyChecks=0,restarts=0,failedSelections=0;U maxPool=0,maxRadius=0;};
std::vector<Stats> statistics;
std::atomic<int> errors{0};

bool baseMember(U x){U p=x/HALF,s=x%HALF;return (members[size_t(p)*WORDS+s/64]>>(s%64))&1;}
bool elapsed(){return stopSearch.load(std::memory_order_relaxed)||std::chrono::steady_clock::now()>=endTime;}
void initialiseCounts(){
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
    initialCounts.swap(a);std::vector<uint16_t>().swap(b);std::vector<uint16_t>().swap(a);
    initialGroupSize.assign(HALF,0);for(U x:baseline){require(initialCounts[x]==1,"baseline independence invariant");initialGroupSize[x/HALF]++;}
    Q zero=0;
    for(U x=0;x<SPACE;x++){
        if(!initialCounts[x])zero++;
        if(baseMember(x))continue;
        if(initialCounts[x]==1){Key k=actualBlockers(x);require(k.n==1,"initial private blocker mismatch");initialPrivate.emplace_back(x,baseline[k.v[0]]);}
        else if(initialCounts[x]>=2&&initialCounts[x]<=8)initialLow.push_back(x);
    }
    require(zero==0,"baseline is not maximal");
    std::cout<<"PLATEAU_INITIAL private_points="<<initialPrivate.size()<<" low_points="<<initialLow.size()<<" zero="<<zero<<"\n"<<std::flush;
}
struct Solver {
    Q nodes=0,limit=200000;bool unknown=false;
    std::vector<int> chosen;
    bool dfs(Q available,int need,const std::vector<Q>& graph){
        if(++nodes>limit){unknown=true;return false;}
        if((nodes&1023)==0&&elapsed()){unknown=true;return false;}
        if(need==0)return true;
        while(__builtin_popcountll(available)>=need){
            int v=__builtin_ctzll(available);available&=available-1;chosen.push_back(v);
            if(dfs(available&~graph[v],need-1,graph))return true;
            chosen.pop_back();if(unknown)return false;
        }return false;
    }
    bool dfsLarge(const std::vector<int>& available,int need,const std::vector<U>& pool){
        if(++nodes>limit){unknown=true;return false;}
        if((nodes&1023)==0&&elapsed()){unknown=true;return false;}
        if(need==0)return true;
        for(size_t i=0;i+size_t(need)<=available.size();i++){
            int v=available[i];std::vector<int> rest;
            for(size_t j=i+1;j<available.size();j++)if(!maskConflict(pool[v],pool[available[j]]))rest.push_back(available[j]);
            chosen.push_back(v);if(dfsLarge(rest,need-1,pool))return true;chosen.pop_back();if(unknown)return false;
        }return false;
    }
    std::vector<U> solve(const std::vector<U>& pool,int need){
        nodes=0;unknown=false;chosen.clear();bool ok=false;
        if(pool.size()<size_t(need))return {};
        if(pool.size()<=64){
            std::vector<Q> graph(pool.size());
            for(size_t i=0;i<pool.size();i++)for(size_t j=i+1;j<pool.size();j++)if(maskConflict(pool[i],pool[j])){graph[i]|=Q(1)<<j;graph[j]|=Q(1)<<i;}
            Q full=pool.size()==64?~Q(0):(Q(1)<<pool.size())-1;ok=dfs(full,need,graph);
        }else{
            std::vector<int> all(pool.size());std::iota(all.begin(),all.end(),0);ok=dfsLarge(all,need,pool);
        }
        if(!ok)return {};std::vector<U> answer;for(int i:chosen)answer.push_back(pool[i]);return answer;
    }
};
struct Chain {
    int id;std::mt19937_64 rng;
    std::vector<uint16_t> counts;
    std::vector<Q> occupied;
    std::vector<int> groupSize;
    std::vector<U> singles;
    std::unordered_map<U,std::vector<U>> privateGroups;
    std::deque<U> nearbyLow;
    std::set<U> removed,added;
    std::unordered_map<U,Q> lastMoved;
    Stats stats;
    std::vector<U> pendingZeros,pendingOnes;
    Q cardinality=134753;
    Chain(int i):id(i),rng(mix(20261030ULL+Q(i)*982451653ULL)){reset();}
    bool contains(U x)const{U p=x/HALF,s=x%HALF;return (occupied[size_t(p)*WORDS+s/64]>>(s%64))&1;}
    void reset(){
        counts=initialCounts;occupied=members;groupSize=initialGroupSize;
        removed.clear();added.clear();lastMoved.clear();nearbyLow.clear();singles.clear();privateGroups.clear();
        singles.reserve(initialPrivate.size()*2);privateGroups.reserve(initialPrivate.size()*2);
        for(auto p:initialPrivate){singles.push_back(p.first);privateGroups[p.second].push_back(p.first);}
        pendingZeros.clear();pendingOnes.clear();cardinality=baseline.size();
    }
    std::vector<U> blockers(U x)const{
        std::vector<U> out;U p=x/HALF,s=x%HALF;const Q* nm=&nbmask[size_t(s)*WORDS];
        for(U q:neighbours[p])if(groupSize[q]){
            const Q* mp=&occupied[size_t(q)*WORDS];
            for(U w=0;w<WORDS;w++){
                Q bits=mp[w]&nm[w];while(bits){U bit=__builtin_ctzll(bits);bits&=bits-1;out.push_back(q*HALF+w*64+bit);}
            }
        }return out;
    }
    void toggleMember(U x,bool present){
        U p=x/HALF,s=x%HALF;Q bit=Q(1)<<(s%64);
        require(contains(x)!=present,"duplicate member update");
        if(present){occupied[size_t(p)*WORDS+s/64]|=bit;groupSize[p]++;cardinality++;}
        else{occupied[size_t(p)*WORDS+s/64]&=~bit;groupSize[p]--;cardinality--;}
        if(baseMember(x)){if(present)removed.erase(x);else removed.insert(x);}
        else{if(present)added.insert(x);else added.erase(x);}
        lastMoved[x]=stats.moves+1;
    }
    void updateCounts(U x,int delta){
        U p=x/HALF,s=x%HALF;
        for(U q:neighbours[p])for(U t:neighbours[s]){
            U y=q*HALF+t;auto& c=counts[y];
            if(delta<0){require(c>0,"negative blocker count");--c;}
            else{require(c<59049,"blocker count overflow");++c;}
            if(c==0)pendingZeros.push_back(y);
            if(c==1)pendingOnes.push_back(y);
            if(c>=2&&c<=8){nearbyLow.push_back(y);if(nearbyLow.size()>65536)nearbyLow.pop_front();}
        }
    }
    void apply(const std::vector<U>& D,const std::vector<U>& A){
        for(U x:D)require(contains(x),"remove nonmember");
        for(size_t i=0;i<A.size();i++){
            for(U b:blockers(A[i]))require(std::find(D.begin(),D.end(),b)!=D.end(),"undeleted blocker in move");
            for(size_t j=0;j<i;j++)require(!directConflict(A[i],A[j]),"move additions are adjacent");
        }
        pendingZeros.clear();pendingOnes.clear();Q before=cardinality;
        for(U x:D)if(std::find(A.begin(),A.end(),x)==A.end()){toggleMember(x,false);updateCounts(x,-1);}
        for(U x:A)if(std::find(D.begin(),D.end(),x)==D.end()){
            require(counts[x]==0,"addition count is not zero");toggleMember(x,true);updateCounts(x,1);
        }
        require(cardinality==before-D.size()+A.size(),"cardinality update mismatch");
        require(cardinality==baseline.size()-removed.size()+added.size(),"delta representation mismatch");
        stats.moves++;
        if(cardinality==baseline.size()){
            stats.maxRadius=std::max(stats.maxRadius,U(removed.size()));
            if(int(removed.size())>bestRadius.load()){
                std::lock_guard<std::mutex> lock(outputMutex);
                if(int(removed.size())>bestRadius.load()){bestRadius=int(removed.size());bestEqual=takeSnapshot();}
            }
        }
    }
    Snapshot takeSnapshot()const{return Snapshot{std::vector<U>(removed.begin(),removed.end()),std::vector<U>(added.begin(),added.end()),id};}
    void publishImprovement(){
        require(cardinality>baseline.size(),"no actual gain");
        std::lock_guard<std::mutex> lock(outputMutex);
        if(!stopSearch){breakthrough=takeSnapshot();stopSearch=true;}
    }
    bool afterMove(){
        if(cardinality>baseline.size()){publishImprovement();return true;}
        for(U x:pendingZeros)if(counts[x]==0&&!contains(x)){
            apply({}, {x});publishImprovement();return true;
        }
        std::sort(pendingOnes.begin(),pendingOnes.end());pendingOnes.erase(std::unique(pendingOnes.begin(),pendingOnes.end()),pendingOnes.end());
        for(U x:pendingOnes)if(counts[x]==1&&!contains(x)){
            auto bl=blockers(x);require(bl.size()==1,"private point count mismatch");U r=bl[0];
            auto& list=privateGroups[r];
            for(U y:list)if(x!=y&&counts[y]==1&&!contains(y)&&!maskConflict(x,y)){
                auto by=blockers(y);require(by.size()==1,"cached private count mismatch");
                if(by[0]==r){apply({r},{x,y});publishImprovement();return true;}
            }
            list.push_back(x);singles.push_back(x);
        }
        return false;
    }
    void compactSingles(){
        std::sort(singles.begin(),singles.end());singles.erase(std::unique(singles.begin(),singles.end()),singles.end());
        std::vector<U> clean;privateGroups.clear();
        for(U x:singles)if(counts[x]==1&&!contains(x)){
            auto bl=blockers(x);require(bl.size()==1,"private rebuild count mismatch");
            clean.push_back(x);privateGroups[bl[0]].push_back(x);
        }singles.swap(clean);
    }
    void consistency(){
        for(int i=0;i<6;i++){
            U x=(i<2&&!singles.empty())?singles[rng()%singles.size()]:U(rng()%SPACE);
            auto bl=blockers(x);require(bl.size()==counts[x],"incremental counts disagree with reconstructed blockers");stats.consistencyChecks++;
        }
        for(U x:added){require(contains(x)&&counts[x]==1,"added member independence invariant");}
        require(removed.size()==added.size(),"neutral state is not cardinality preserving");
    }
    bool singleStep(){
        if(singles.empty())return false;
        for(int tries=0;tries<128;tries++){
            U q=singles[rng()%singles.size()];if(contains(q)||counts[q]!=1)continue;
            auto bl=blockers(q);require(bl.size()==1,"selected private blocker mismatch");U r=bl[0];
            auto it=lastMoved.find(r);
            if(it!=lastMoved.end()&&stats.moves<it->second+12)continue;
            apply({r},{q});stats.singleMoves++;afterMove();return true;
        }return false;
    }
    std::vector<U> poolAfter(const std::vector<U>& D){
        std::vector<U> out=D;
        for(U r:D)for(U q:neighbours[r/HALF])for(U t:neighbours[r%HALF]){
            U x=q*HALF+t;auto m=counts[x];if(m>D.size()||contains(x))continue;
            unsigned removedCount=0;for(U d:D)removedCount+=maskConflict(x,d);
            if(removedCount==m)out.push_back(x);
        }
        std::sort(out.begin(),out.end());out.erase(std::unique(out.begin(),out.end()),out.end());return out;
    }
    void blockStep(){
        U x=NIL;
        for(int tries=0;tries<64;tries++){
            U q=(!nearbyLow.empty()&&rng()%4)?nearbyLow[rng()%nearbyLow.size()]:initialLow[rng()%initialLow.size()];
            if(!contains(q)&&counts[q]>=2&&counts[q]<=8){x=q;break;}
        }
        if(x==NIL)return;auto D=blockers(x);require(D.size()==counts[x],"block trial count mismatch");
        auto pool=poolAfter(D);stats.blockTrials++;stats.maxPool=std::max(stats.maxPool,U(pool.size()));
        Solver s;auto gain=s.solve(pool,int(D.size())+1);
        if(!gain.empty()){stats.exactPools++;apply(D,gain);publishImprovement();return;}
        if(s.unknown){stats.unknownPools++;return;}stats.exactPools++;
        // Prefer at least one outside point; the original deletion set stays as a fallback.
        std::shuffle(pool.begin(),pool.end(),rng);
        std::stable_partition(pool.begin(),pool.end(),[&](U p){return !contains(p);});
        auto equal=s.solve(pool,int(D.size()));if(s.unknown){stats.unknownPools++;return;}
        require(!equal.empty(),"original set missing from repair pool");
        bool changed=false;for(U q:equal)if(!contains(q))changed=true;
        if(changed){apply(D,equal);stats.blockMoves++;afterMove();}
    }
    void run(){
        while(!elapsed()){
            if(!singleStep())stats.failedSelections++;
            if(elapsed())break;
            blockStep();
            if(elapsed())break;
            if((stats.moves&31)==0)consistency();
            if(singles.size()>20000||((stats.moves&255)==0))compactSingles();
            if(int(removed.size())>=radiusLimit){consistency();stats.restarts++;reset();}
            if(stats.failedSelections>1000&&stats.failedSelections%1000==0){stats.restarts++;reset();}
        }
        if(cardinality==baseline.size())consistency();
    }
};
void solverTests(){
    endTime=std::chrono::steady_clock::now()+std::chrono::seconds(120);
    std::mt19937 rng(761923);
    for(int trial=0;trial<500;trial++){
        int n=2+rng()%11;std::vector<Q> g(n);
        for(int i=0;i<n;i++)for(int j=i+1;j<n;j++)if(rng()%2){g[i]|=Q(1)<<j;g[j]|=Q(1)<<i;}
        int exact=0;
        for(unsigned m=0;m<(1u<<n);m++){
            bool ok=true;for(int i=0;i<n;i++)if(m&(1u<<i))if(g[i]&m){ok=false;break;}
            if(ok)exact=std::max(exact,__builtin_popcount(m));
        }
        for(int need=1;need<=9;need++){
            Solver s;bool answer=s.dfs((Q(1)<<n)-1,need,g);
            require(!s.unknown&&answer==(exact>=need),"bounded solver failed brute-force self-test");
        }
    }
    Solver s;s.limit=0;std::vector<Q> g(8);require(!s.dfs(255,6,g)&&s.unknown,"node limit must be UNKNOWN, not negative");
    std::cout<<"PLATEAU_SELFTEST graphs=500 targets=9 node_limit_reporting=PASS\n"<<std::flush;
}
void printSnapshot(const Snapshot& s,const char* kind){
    if(s.chain<0)return;
    std::cout<<"SNAPSHOT {\"kind\":\""<<kind<<"\",\"chain\":"<<s.chain<<",\"removed\":[";
    for(size_t i=0;i<s.removed.size();i++){if(i)std::cout<<',';std::cout<<s.removed[i];}
    std::cout<<"],\"added\":[";for(size_t i=0;i<s.added.size();i++){if(i)std::cout<<',';std::cout<<s.added[i];}
    std::cout<<"]}\n"<<std::flush;
}
}
int main(int argc,char** argv){
    using namespace plateau;
    try{
        std::string input="/tmp/p004_points.txt";bool selfOnly=false;
        for(int i=1;i<argc;i++){
            std::string a=argv[i];
            if(a=="--self-test")selfOnly=true;
            else if(a=="--chains"&&i+1<argc)chains=std::stoi(argv[++i]);
            else if(a=="--seconds"&&i+1<argc)duration=std::stoi(argv[++i]);
            else if(a=="--radius"&&i+1<argc)radiusLimit=std::stoi(argv[++i]);
            else if(a=="--input"&&i+1<argc)input=argv[++i];
            else throw std::runtime_error("invalid plateau argument");
        }
        require(chains>=1&&chains<=16,"chains must be 1..16");require(duration>=1&&duration<=300,"duration must be 1..300");require(radiusLimit>=16&&radiusLimit<=1024,"radius must be 16..1024");
        omp_set_dynamic(0);omp_set_num_threads(chains);selftest();solverTests();if(selfOnly)return 0;
        load(input);initialiseCounts();statistics.resize(chains);
        endTime=std::chrono::steady_clock::now()+std::chrono::seconds(duration);
        #pragma omp parallel for schedule(static,1)
        for(int id=0;id<chains;id++){
            try{Chain c(id);c.run();statistics[id]=c.stats;}
            catch(const std::exception& e){errors++;stopSearch=true;std::lock_guard<std::mutex> lock(outputMutex);std::cerr<<"CHAIN_ERROR id="<<id<<" message="<<e.what()<<'\n';}
        }
        for(int i=0;i<chains;i++){
            auto s=statistics[i];std::cout<<"CHAIN id="<<i<<" accepted_moves="<<s.moves<<" single_moves="<<s.singleMoves<<" block_moves="<<s.blockMoves<<" block_trials="<<s.blockTrials<<" completed_pool_checks="<<s.exactPools<<" solver_limits="<<s.unknownPools<<" consistency_checks="<<s.consistencyChecks<<" restarts="<<s.restarts<<" max_radius="<<s.maxRadius<<" max_pool="<<s.maxPool<<"\n";
        }
        printSnapshot(bestEqual,"neutral");printSnapshot(breakthrough,"candidate");
        std::cout<<"PLATEAU_RESULT {\"status\":\""<<(errors?"incomplete_error":(breakthrough.chain>=0?"candidate":"budget_finished"))<<"\",\"chains\":"<<chains<<",\"time_budget_seconds\":"<<duration<<",\"best_neutral_radius\":"<<bestRadius.load()<<",\"errors\":"<<errors.load()<<",\"exhaustive\":false}\n";
        return errors?3:0;
    }catch(const std::exception& e){std::cerr<<"INCOMPLETE ERROR "<<e.what()<<'\n';return 3;}
}
