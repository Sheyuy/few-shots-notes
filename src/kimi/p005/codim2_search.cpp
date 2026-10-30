#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <tuple>
#include <vector>

static constexpr int H=88573;
static constexpr int W=43;
using Mask=std::array<std::uint64_t,W>;
static std::vector<Mask> masks;

static std::pair<int,int> best_response(int b,std::mt19937_64 &rng) {
    int best=-1,choice=-1;
    std::uint64_t ties=0;
    for(int a=0;a<H;a++) {
        if(a==b) continue; // The two functionals must be projectively independent.
        int n=0;
        for(int k=0;k<W;k++) n+=std::popcount(masks[a][k]&masks[b][k]);
        if(n>best) {
            best=n; choice=a; ties=1;
        } else if(n==best) {
            ++ties;
            if(std::uniform_int_distribution<std::uint64_t>(1,ties)(rng)==1) choice=a;
        }
    }
    return {choice,best};
}

int main(int argc,char **argv) {
    if(argc<5) {
        std::cerr<<"usage: codim2_search masks.bin restarts seed output.txt [plateau]\n";
        return 2;
    }
    int restarts=std::stoi(argv[2]);
    std::uint64_t seed=std::stoull(argv[3]);
    std::string output=argv[4];
    int plateau=argc>5?std::stoi(argv[5]):3;
    masks.resize(H);
    std::ifstream in(argv[1],std::ios::binary);
    if(!in.read(reinterpret_cast<char*>(masks.data()),std::streamsize(H*sizeof(Mask)))) {
        std::cerr<<"cannot read masks\n"; return 2;
    }
    std::mt19937_64 rng(seed);
    std::vector<std::tuple<int,int,int>> local;
    int global=-1,ga=-1,gb=-1;
    auto start=std::chrono::steady_clock::now();
    for(int r=0;r<restarts;r++) {
        int a=std::uniform_int_distribution<int>(0,H-1)(rng);
        int b=std::uniform_int_distribution<int>(0,H-1)(rng);
        if(a==b) b=(b+1)%H;
        int current=-1,stale=0;
        for(int step=0;step<30;step++) {
            auto na=best_response(b,rng); a=na.first;
            auto nb=best_response(a,rng); b=nb.first;
            int score=nb.second;
            if(score>global) {
                global=score;ga=a;gb=b;
                std::cerr<<"global "<<global<<" pair "<<ga<<" "<<gb<<" restart "<<r<<"\n";
            }
            if(score>current) {current=score;stale=0;}
            else if(++stale>=plateau) break;
        }
        if(current>=450) local.emplace_back(current,a,b);
        if((r+1)%100==0) {
            double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
            std::cerr<<"progress "<<r+1<<"/"<<restarts<<" global "<<global<<" seconds "<<sec<<"\n";
        }
    }
    std::sort(local.begin(),local.end(),std::greater<>());
    std::ofstream out(output);
    out<<"global "<<global<<" "<<ga<<" "<<gb<<"\n";
    for(auto [score,a,b]:local) out<<score<<" "<<a<<" "<<b<<"\n";
    std::cout<<"RESULT "<<global<<" "<<ga<<" "<<gb<<" local_records "<<local.size()<<"\n";
}
