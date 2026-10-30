#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
using namespace std;
static const int N=6561;static uint8_t hi[N],lo[N],add4[81][81],sub4[81][81];
static inline int add3(int a,int b){return add4[hi[a]][hi[b]]*81+add4[lo[a]][lo[b]];}
static inline int sub3(int a,int b){return sub4[hi[a]][hi[b]]*81+sub4[lo[a]][lo[b]];}
static uint64_t hash_points(const vector<int>&p){uint64_t h=1469598103934665603ULL;for(int x:p){h^=(uint64_t)x;h*=1099511628211ULL;}return h;}
int main(int argc,char**argv){
 if(argc<4){cerr<<"usage: subspace4_cuts poolfile cuts3 outfile [min_occupancy]\n";return 2;}int threshold=argc>4?atoi(argv[4]):36;
 for(int x=0;x<N;x++){hi[x]=x/81;lo[x]=x%81;}for(int a=0;a<81;a++)for(int b=0;b<81;b++){int aa=a,bb=b,s=0,d=0,p=1;for(int i=0;i<4;i++){s+=(aa%3+bb%3)%3*p;d+=(aa%3+9-bb%3)%3*p;aa/=3;bb/=3;p*=3;}add4[a][b]=s;sub4[a][b]=d;}
 vector<int> pool;string line;ifstream in(argv[1]);while(getline(in,line)){if(line.empty()||line[0]=='#')continue;pool.push_back(stoi(line));}sort(pool.begin(),pool.end());pool.erase(unique(pool.begin(),pool.end()),pool.end());int m=pool.size();vector<int>idx(N,-1);for(int i=0;i<m;i++)idx[pool[i]]=i;
 ifstream in3(argv[2]);ofstream out(argv[3]);out<<"# affine4_bound 20 min_pool_occupancy "<<threshold<<"\n";
 unordered_set<uint64_t>seen;seen.reserve(1<<20);long long source=0,bad=0,examined=0,cuts=0;auto t0=chrono::steady_clock::now();
 while(getline(in3,line)){
  if(line.empty()||line[0]=='#')continue;istringstream ss(line);int n;ss>>n;vector<int>vs(n);for(int&i:vs)ss>>i;
  vector<int>u=vs;sort(u.begin(),u.end());if(unique(u.begin(),u.end())!=u.end()||n<18)continue;source++;
  int p0=pool[vs[0]],p1=-1,p2=-1,p3=-1;for(int i=1;i<n;i++)if(pool[vs[i]]!=p0){p1=pool[vs[i]];break;}if(p1<0){bad++;continue;}
  int d1=sub3(p1,p0),dn1=sub3(0,d1);
  for(int i=1;i<n;i++){int w=sub3(pool[vs[i]],p0);if(w!=0&&w!=d1&&w!=dn1){p2=pool[vs[i]];break;}}if(p2<0){bad++;continue;}
  int d2=sub3(p2,p0),dn2=sub3(0,d2),plane[9];for(int a=0;a<3;a++)for(int b=0;b<3;b++){int ad=a==0?0:(a==1?d1:dn1),bd=b==0?0:(b==1?d2:dn2);plane[3*a+b]=add3(p0,add3(ad,bd));}
  for(int i=1;i<n;i++){int q=pool[vs[i]];bool inside=false;for(int j=0;j<9;j++)if(q==plane[j])inside=true;if(!inside){p3=q;break;}}if(p3<0){bad++;continue;}
  int d3=sub3(p3,p0),dn3=sub3(0,d3),flat[27];for(int c=0;c<3;c++){int cd=c==0?0:(c==1?d3:dn3);for(int j=0;j<9;j++)flat[9*c+j]=add3(plane[j],cd);}
  for(int v:vs){bool inside=false;for(int j=0;j<27;j++)if(pool[v]==flat[j])inside=true;if(!inside){bad++;goto next_source;}}
  for(int y:pool){bool inside=false;for(int j=0;j<27;j++)if(y==flat[j]){inside=true;break;}if(inside)continue;int w=sub3(y,p0),wn=sub3(0,w),pts[81],vars[81],nv=0;
   for(int r=0;r<3;r++){int rw=r==0?0:(r==1?w:wn);for(int j=0;j<27;j++){int p=add3(flat[j],rw);pts[27*r+j]=p;if(idx[p]>=0)vars[nv++]=idx[p];}}
   examined++;if(nv<threshold)continue;sort(pts,pts+81);vector<int>key(pts,pts+81);if(!seen.insert(hash_points(key)).second)continue;sort(vars,vars+nv);out<<nv;for(int i=0;i<nv;i++)out<<' '<<vars[i];out<<'\n';cuts++;
  }
  next_source:;
 }
 cerr<<"DONE source "<<source<<" bad "<<bad<<" examined "<<examined<<" cuts "<<cuts<<" seconds "<<chrono::duration<double>(chrono::steady_clock::now()-t0).count()<<"\n";
}
