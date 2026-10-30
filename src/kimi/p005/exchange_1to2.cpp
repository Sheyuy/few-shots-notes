#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;
static int third4[81][81];
static inline int third(int a,int b){return third4[a/81][b/81]*81+third4[a%81][b%81];}
static vector<int> load(const string&path){
    ifstream f(path);if(!f)throw runtime_error("cannot open "+path);
    vector<int> v;string line;
    while(getline(f,line)){if(line.empty()||line[0]=='#')continue;v.push_back(stoi(line));}
    return v;
}
static bool valid_cap(const vector<int>&S){
    vector<char> mem(6561,0);for(int x:S){if(x<0||x>=6561||mem[x])return false;mem[x]=1;}
    for(int i=0;i<(int)S.size();i++)for(int j=i+1;j<(int)S.size();j++)if(mem[third(S[i],S[j])])return false;
    return true;
}
static void save(const string&path,const vector<int>&S){
    ofstream f(path);f<<"# size "<<S.size()<<"\n";for(int x:S)f<<x<<'\n';
}
int main(int argc,char**argv){
    if(argc<3){cerr<<"usage: exchange_1to2 poolfile seed...\n";return 2;}
    for(int a=0;a<81;a++)for(int b=0;b<81;b++){
        int x=a,y=b,z=0,p=1;for(int k=0;k<4;k++){z+=(6-x%3-y%3)%3*p;x/=3;y/=3;p*=3;}third4[a][b]=z;
    }
    const char* outprefix=getenv("OUTPUT_PREFIX");if(!outprefix)outprefix="exchange-1to2";
    vector<int> pool=load(argv[1]);vector<char> allowed(6561,0);for(int x:pool)allowed[x]=1;
    for(int si=2;si<argc;si++){
        vector<int> B=load(argv[si]);sort(B.begin(),B.end());
        if(!valid_cap(B)){cerr<<"invalid seed "<<argv[si]<<"\n";return 3;}
        vector<char> mem(6561,0);for(int x:B)mem[x]=1;
        vector<vector<pair<int,int>>> edges(6561);
        for(int i=0;i<(int)B.size();i++)for(int j=i+1;j<(int)B.size();j++){
            int z=third(B[i],B[j]);if(!mem[z]&&allowed[z])edges[z].push_back({B[i],B[j]});
        }
        vector<int> out;for(int x:pool)if(!mem[x])out.push_back(x);
        uint64_t pairs=0,individually_possible=0;bool found=false;
        for(int ix=0;ix<(int)out.size()&&!found;ix++)for(int iy=ix+1;iy<(int)out.size()&&!found;iy++){
            int x=out[ix],y=out[iy];pairs++;
            int mandatory=third(x,y);if(!mem[mandatory])mandatory=-1;
            int first=-1,ea=-1,eb=-1;
            if(mandatory>=0){first=mandatory;ea=eb=mandatory;}
            else if(!edges[x].empty()){first=1;ea=edges[x][0].first;eb=edges[x][0].second;}
            else if(!edges[y].empty()){first=1;ea=edges[y][0].first;eb=edges[y][0].second;}
            int cand[2]={ea,eb};
            for(int ci=0;ci<2&&!found;ci++){
                int b=cand[ci];if(b<0)continue;
                if(ci==1&&b==cand[0])continue;
                if(mandatory>=0&&b!=mandatory)continue;
                bool ok=true;
                for(auto e:edges[x])if(e.first!=b&&e.second!=b){ok=false;break;}
                if(ok)for(auto e:edges[y])if(e.first!=b&&e.second!=b){ok=false;break;}
                if(!ok)continue;
                vector<int> C;C.reserve(B.size()+2);
                for(int z:B)if(z!=b)C.push_back(z);C.push_back(x);C.push_back(y);sort(C.begin(),C.end());
                if(C.size()<B.size()+1||!valid_cap(C)){cerr<<"internal construction error\n";return 4;}
                string path=string(outprefix)+"-seed"+to_string(si-2)+"-n"+to_string(C.size())+".txt";
                save(path,C);cout<<"FOUND seed "<<argv[si]<<" add "<<x<<" "<<y<<" delete "<<b<<" size "<<C.size()<<" file "<<path<<endl;
                found=true;
            }
            if(first<0){
                // No deletion is needed: x and y can simply be added.
                vector<int> C=B;C.push_back(x);C.push_back(y);sort(C.begin(),C.end());
                if(!valid_cap(C)){cerr<<"internal construction error\n";return 4;}
                string path=string(outprefix)+"-seed"+to_string(si-2)+"-n"+to_string(C.size())+".txt";
                save(path,C);cout<<"FOUND seed "<<argv[si]<<" add "<<x<<" "<<y<<" delete none size "<<C.size()<<" file "<<path<<endl;found=true;
            }
        }
        cout<<"DONE seed "<<si-2<<" base "<<B.size()<<" outside "<<out.size()<<" pairs "<<pairs<<" found "<<found<<endl;
    }
    return 0;
}
