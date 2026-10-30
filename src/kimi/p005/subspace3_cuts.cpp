
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
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
static uint64_t hash_points(const int *p,int n){uint64_t h=1469598103934665603ULL;for(int i=0;i<n;i++){h^=(uint64_t)p[i];h*=1099511628211ULL;}return h;}
int main(int argc,char**argv){
 if(argc<3){cerr<<"usage: subspace3_cuts poolfile outfile\n";return 2;}
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
 vector<Line> lines;for(int i=0;i<m;i++)for(int j=i+1;j<m;j++){int z=sub3(0,add3(pool[i],pool[j])),k=idx[z];if(k>j)lines.push_back({pool[i],pool[j],z});}
 auto t0=chrono::steady_clock::now();cerr<<"pool "<<m<<" lines "<<lines.size()<<"\n";
 vector<array<int,9>> planes;unordered_set<uint64_t> seen2;seen2.reserve(1<<20);
 for(auto &L:lines)for(int x:pool){
  if(x==L.a||x==L.b||x==L.c)continue;
  int d=sub3(x,L.a),dn=sub3(0,d),dt[3]={0,d,dn},lp[3]={L.a,L.b,L.c},pts[9],vars[9],nv=0;
  for(int t=0;t<3;t++)for(int s=0;s<3;s++){int p=add3(lp[s],dt[t]);pts[3*t+s]=p;if(idx[p]>=0)vars[nv++]=idx[p];}
  if(nv<9)continue;
  int sorted[9];copy(pts,pts+9,sorted);sort(sorted,sorted+9);
  if(!seen2.insert(hash_points(sorted,9)).second)continue;
  array<int,9> a;copy(pts,pts+9,a.begin());planes.push_back(a);
 }
 cerr<<"strong planes "<<planes.size()<<"\n";
 ofstream out(argv[2]);out<<"# pool "<<m<<" affine3_bound 9 source_planes "<<planes.size()<<"\n";
 unordered_set<uint64_t> seen3;seen3.reserve(1<<20);long long examined=0,cuts=0;
 for(auto &pl:planes)for(int y:pool){
  bool in_plane=false;for(int q=0;q<9;q++)if(y==pl[q]){in_plane=true;break;}if(in_plane)continue;
  int w=sub3(y,pl[0]),wn=sub3(0,w),ws[3]={0,w,wn},pts[27],vars[27],nv=0;
  for(int r=0;r<3;r++)for(int q=0;q<9;q++){int p=add3(pl[q],ws[r]);pts[9*r+q]=p;if(idx[p]>=0)vars[nv++]=idx[p];}
  examined++;if(nv<10)continue;
  sort(pts,pts+27);if(!seen3.insert(hash_points(pts,27)).second)continue;
  sort(vars,vars+nv);out<<nv;for(int i=0;i<nv;i++)out<<' '<<vars[i];out<<'\n';cuts++;
  if(cuts%100000==0)cerr<<"cuts "<<cuts<<" examined "<<examined<<"\n";
 }
 double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
 cerr<<"DONE examined "<<examined<<" cuts "<<cuts<<" seconds "<<sec<<"\n";
}
