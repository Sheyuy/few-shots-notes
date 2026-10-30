#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>
using namespace std;
static const int N=6561;
static uint8_t hi[N],lo[N],add4[81][81],sub4[81][81];
static inline int add3(int a,int b){return add4[hi[a]][hi[b]]*81+add4[lo[a]][lo[b]];}
static inline int sub3(int a,int b){return sub4[hi[a]][hi[b]]*81+sub4[lo[a]][lo[b]];}
struct Line{int a,b,c;};
int main(int argc,char**argv){
 if(argc<3){cerr<<"usage: plane_cuts poolfile outfile\n";return 2;}
 for(int x=0;x<N;x++){hi[x]=x/81;lo[x]=x%81;}
 for(int a=0;a<81;a++)for(int b=0;b<81;b++){
  int aa=a,bb=b,s=0,d=0,p=1;
  for(int i=0;i<4;i++){s+=(aa%3+bb%3)%3*p;d+=(aa%3+9-bb%3)%3*p;aa/=3;bb/=3;p*=3;}
  add4[a][b]=s;sub4[a][b]=d;
 }
 vector<int> pool;string line;ifstream in(argv[1]);
 while(getline(in,line)){if(line.empty()||line[0]=='#')continue;pool.push_back(stoi(line));}
 sort(pool.begin(),pool.end());pool.erase(unique(pool.begin(),pool.end()),pool.end());int m=pool.size();
 vector<int> idx(N,-1);for(int i=0;i<m;i++)idx[pool[i]]=i;
 vector<Line> lines;
 for(int i=0;i<m;i++)for(int j=i+1;j<m;j++){
  int z=sub3(0,add3(pool[i],pool[j])),k=idx[z];if(k>j)lines.push_back({pool[i],pool[j],z});
 }
 auto t0=chrono::steady_clock::now();cerr<<"pool "<<m<<" lines "<<lines.size()<<"\n";
 ofstream out(argv[2]);out<<"# pool "<<m<<" lines "<<lines.size()<<" plane_bound 4\n";
 unordered_set<uint64_t> seen;seen.reserve(1<<20);long long examined=0,candidates=0,cuts=0;
 for(auto &L:lines)for(int xi=0;xi<m;xi++){
  int x=pool[xi];if(x==L.a||x==L.b||x==L.c)continue;
  int d=sub3(x,L.a),dn=sub3(0,d),dt[3]={0,d,dn},lp[3]={L.a,L.b,L.c},pts[9],vars[9],nv=0;
  for(int t=0;t<3;t++)for(int s=0;s<3;s++){int p=add3(lp[s],dt[t]);pts[3*t+s]=p;if(idx[p]>=0)vars[nv++]=idx[p];}
  examined++;if(nv<5)continue;candidates++;
  sort(pts,pts+9);uint64_t h=1469598103934665603ULL;for(int p:pts){h^=(uint64_t)p;h*=1099511628211ULL;}
  if(!seen.insert(h).second)continue;
  if(nv>=8){sort(vars,vars+nv);out<<nv;for(int i=0;i<nv;i++)out<<' '<<vars[i];out<<'\n';cuts++;}
  if(cuts%100000==0)cerr<<"cuts "<<cuts<<" examined "<<examined<<"\n";
 }
 double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
 cerr<<"DONE examined "<<examined<<" candidates "<<candidates<<" cuts "<<cuts<<" seconds "<<sec<<"\n";
 return 0;
}
