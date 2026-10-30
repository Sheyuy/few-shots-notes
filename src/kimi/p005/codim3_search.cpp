#include <algorithm>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

static constexpr int H=265720,D=12,P=6464,W=101;
static std::vector<std::uint64_t> masks;
static std::vector<std::uint8_t> F;
static std::unordered_map<int,int> findex;

static int encode(const std::uint8_t *v) {
    int code=0,p=1;
    for(int r=0;r<D;r++){code+=v[r]*p;p*=3;}
    return code;
}
static int combination(int b,int c,int s) {
    std::uint8_t v[D];int scale=1;
    for(int r=0;r<D;r++)v[r]=(F[b*D+r]+s*F[c*D+r])%3;
    for(int r=0;r<D;r++)if(v[r]){scale=v[r]==1?1:2;break;}
    int code=0,p=1;
    for(int r=0;r<D;r++){code+=((v[r]*scale)%3)*p;p*=3;}
    auto it=findex.find(code);
    return it==findex.end()?-1:it->second;
}
static std::pair<int,int> best_response(int b,int c,std::mt19937_64 &rng) {
    std::uint64_t base[W];
    const std::uint64_t *mb=masks.data()+(std::size_t)b*W,*mc=masks.data()+(std::size_t)c*W;
    for(int k=0;k<W;k++)base[k]=mb[k]&mc[k];
    int dep0=b,dep1=c,dep2=combination(b,c,1),dep3=combination(b,c,2);
    int best=-1,choice=-1;std::uint64_t ties=0;
    for(int a=0;a<H;a++) {
        if(a==dep0||a==dep1||a==dep2||a==dep3)continue;
        const std::uint64_t *ma=masks.data()+(std::size_t)a*W;
        int n=0;for(int k=0;k<W;k++)n+=std::popcount(ma[k]&base[k]);
        if(n>best){best=n;choice=a;ties=1;}
        else if(n==best){++ties;if(std::uniform_int_distribution<std::uint64_t>(1,ties)(rng)==1)choice=a;}
    }
    return {choice,best};
}
int main(int argc,char **argv) {
    if(argc<6){std::cerr<<"usage: codim3_search masks.bin functionals.bin restarts seed output.txt [plateau]\n";return 2;}
    int restarts=std::stoi(argv[3]),plateau=argc>6?std::stoi(argv[6]):3;
    std::uint64_t seed=std::stoull(argv[4]);std::string output=argv[5];
    masks.resize((std::size_t)H*W);F.resize(H*D);
    std::string base=argv[1];constexpr int PARTS=8;int chunk=(H+PARTS-1)/PARTS;
    for(int part=0;part<PARTS;part++){
        int begin=part*chunk,count=std::min(chunk,H-begin);if(count<=0)break;
        std::ifstream in(base+"."+std::to_string(part),std::ios::binary);
        if(!in.read(reinterpret_cast<char*>(masks.data()+(std::size_t)begin*W),(std::size_t)count*W*8)){
            std::cerr<<"mask shard error "<<part<<"\n";return 2;
        }
    }
    std::ifstream fi(argv[2],std::ios::binary);
    if(!fi.read(reinterpret_cast<char*>(F.data()),F.size()).good()){std::cerr<<"functional input error\n";return 2;}
    for(int h=0;h<H;h++)findex[encode(F.data()+h*D)]=h;
    std::mt19937_64 rng(seed);int global=-1,ga=-1,gb=-1,gc=-1;
    std::vector<std::tuple<int,int,int,int>> local;
    auto start=std::chrono::steady_clock::now();
    for(int r=0;r<restarts;r++) {
        int a=rng()%H,b=rng()%H,c=rng()%H,current=-1,stale=0;
        for(int step=0;step<30;step++) {
            auto na=best_response(b,c,rng);a=na.first;
            auto nb=best_response(a,c,rng);b=nb.first;
            auto nc=best_response(a,b,rng);c=nc.first;int score=nc.second;
            if(score>global){global=score;ga=a;gb=b;gc=c;std::cerr<<"global "<<global<<" triple "<<ga<<" "<<gb<<" "<<gc<<" restart "<<r<<"\n";}
            if(score>current){current=score;stale=0;}else if(++stale>=plateau)break;
        }
        if(current>=400)local.emplace_back(current,a,b,c);
        if((r+1)%25==0){double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cerr<<"progress "<<r+1<<"/"<<restarts<<" global "<<global<<" seconds "<<sec<<"\n";}
    }
    std::sort(local.begin(),local.end(),std::greater<>());std::ofstream out(output);
    out<<"global "<<global<<" "<<ga<<" "<<gb<<" "<<gc<<"\n";
    for(auto [n,a,b,c]:local)out<<n<<" "<<a<<" "<<b<<" "<<c<<"\n";
    std::cout<<"RESULT "<<global<<" "<<ga<<" "<<gb<<" "<<gc<<" local_records "<<local.size()<<"\n";
}
