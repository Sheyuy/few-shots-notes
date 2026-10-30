#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <omp.h>

int main(int argc,char **argv) {
    if(argc<7) {
        std::cerr<<"usage: build_cap_masks matrix.txt functionals.bin output.bin D H P\n";
        return 2;
    }
    int D=std::stoi(argv[4]),H=std::stoi(argv[5]),P=std::stoi(argv[6]);
    int W=(P+63)/64;
    std::vector<std::uint8_t> M(D*P);
    std::ifstream mi(argv[1]);
    for(int r=0;r<D;r++) {
        std::string s;mi>>s;
        if((int)s.size()!=P){std::cerr<<"bad matrix row "<<r<<"\n";return 2;}
        for(int p=0;p<P;p++)M[r*P+p]=s[p]-'0';
    }
    std::vector<std::uint8_t> F(H*D);
    std::ifstream fi(argv[2],std::ios::binary);
    if(!fi.read(reinterpret_cast<char*>(F.data()),F.size())){std::cerr<<"bad functionals\n";return 2;}
    std::vector<std::uint64_t> masks((std::size_t)H*W);
    #pragma omp parallel for schedule(dynamic,16) num_threads(2)
    for(int h=0;h<H;h++) {
        std::uint64_t *out=masks.data()+(std::size_t)h*W;
        for(int p=0;p<P;p++) {
            int v=0;
            for(int r=0;r<D;r++)v+=F[h*D+r]*M[r*P+p];
            if(v%3==0)out[p/64]|=1ULL<<(p%64);
        }
    }
    std::string base=argv[3];int parts=8,chunk=(H+parts-1)/parts;
    for(int part=0;part<parts;part++) {
        int begin=part*chunk,count=std::min(chunk,H-begin);
        if(count<=0)break;
        std::ofstream out(base+"."+std::to_string(part),std::ios::binary);
        out.write(reinterpret_cast<const char*>(masks.data()+(std::size_t)begin*W),(std::size_t)count*W*sizeof(std::uint64_t));
    }
    std::cout<<"wrote "<<H<<" masks in eight shards, "<<P<<" points, "<<W<<" words\n";
}
