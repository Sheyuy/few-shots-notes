#include <cstdio>
#include <vector>
#include <cstdint>
using namespace std;
static const int N5=16807;
static int dig[N5][5];
int main(){
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    vector<uint16_t> nb[N5];
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
    printf("deg0=%zu\n",nb[0].size());
    int bad=0;
    for(int v=0;v<N5;v++)for(auto u:nb[v])if(dig[u][0]<0)bad++;
    printf("nonsense=%d\n",bad);
    // symmetry check on a sample
    int asym=0;
    for(int v=0;v<2000;v++)for(auto u:nb[v]){
        bool f=false;for(auto w:nb[u])if(w==v){f=true;break;}
        if(!f){asym++;break;}
    }
    printf("asym_sample=%d\n",asym);
    // spot check: nb[0] vs manual
    printf("nb0_size=%zu first=%d\n",nb[0].size(),(int)nb[0][0]);
    return 0;
}
