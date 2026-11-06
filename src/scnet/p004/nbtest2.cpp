#include <cstdio>
#include <vector>
#include <cstdint>
using namespace std;
static const int N5=16807;
static int dig[N5][5];
int main(){
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    vector<vector<uint16_t>> nb(N5);
    for(int v=0;v<N5;v++){
        int d[5];for(int i=0;i<5;i++)d[i]=dig[v][i];
        int idx[5];
        for(idx[0]=-1;idx[0]<=1;idx[0]++)for(idx[1]=-1;idx[1]<=1;idx[1]++)
        for(idx[2]=-1;idx[2]<=1;idx[2]++)for(idx[3]=-1;idx[3]<=1;idx[3]++)
        for(idx[4]=-1;idx[4]<=1;idx[4]++){
            bool any=false;int x=0;
            for(int i=4;i>=0;i--){int c=(d[i]+idx[i]+7)%7;if(idx[i])any=true;x=x*7+c;}
            if(any)nb[v].push_back((uint16_t)x);
        }
    }
    int asym=0,self=0,dup=0;
    for(int v=0;v<N5;v++){
        vector<char> m(N5,0);
        for(auto u:nb[v]){ if(u==v)self++; if(m[u])dup++; m[u]=1; }
        for(auto u:nb[v]){bool f=false;for(auto w:nb[u])if(w==v){f=true;break;} if(!f)asym++;}
    }
    printf("asym=%d self=%d dup=%d\n",asym,self,dup);
    // check degree of a few
    printf("deg[100]=%zu deg[16806]=%zu\n",nb[100].size(),nb[16806].size());
    return 0;
}
