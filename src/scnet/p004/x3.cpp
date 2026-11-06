// Complete 3-exchange check for the 367-point set R in C7^5.
// Look for A (|A|=4, independent, A cap S = empty) with |T(A)| <= 3,
// where T(A) = union over a in A of {s in S : a ~ s}.  If such A exists,
// (S \ R) cup A with T(A) subset R, |R|=3, is an independent 368-point set.
#include <cstdio>
#include <cstdint>
#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <cstring>
using namespace std;

static const int N5=16807;
int dig[N5][5];

static inline bool adj(int a,int b){
    bool any=false;
    for(int i=0;i<5;i++){
        int d=dig[a][i]-dig[b][i]; if(d<0)d=-d; if(d==6)d=1; if(d>1)return false;
        if(d==1)any=true;
    }
    return any;
}
static inline int enc(const int v[5]){int x=0;for(int i=4;i>=0;i--)x=x*7+v[i];return x;}

int main(int argc,char**argv){
    const char* path = argc>1?argv[1]:"/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt";
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r"); if(!f){perror("open");return 1;}
    vector<int> S; int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5) S.push_back(enc(a));
    fclose(f);
    printf("S size %zu\n",S.size());
    vector<char> inS(N5,0); for(int s:S) inS[s]=1;
    // sanity: S independent
    for(size_t i=0;i<S.size();i++)for(size_t j=i+1;j<S.size();j++)
        if(adj(S[i],S[j])){printf("S not independent! %d %d\n",S[i],S[j]);return 1;}
    // T(v) for v outside S, keep those with |T|<=3
    unordered_map<uint64_t, vector<int>> bucket;   // key from sorted triple, -1 padded
    auto key3=[&](int x,int y,int z)->uint64_t{
        int t[3]={x,y,z}; sort(t,t+3);
        return (uint64_t)(t[0]+1)*368ull*368ull + (uint64_t)(t[1]+1)*368ull + (uint64_t)(t[2]+1);
    };
    long long counts[5]={0,0,0,0,0};
    for(int v=0;v<N5;v++){
        if(inS[v])continue;
        int t[3]={-1,-1,-1}; int n=0;
        for(int s:S) if(adj(v,s)){ if(n<3)t[n]=s; n++; if(n>3)break; }
        if(n>3)continue;
        counts[n]++;
        bucket[key3(t[0],t[1],t[2])].push_back(v);
    }
    printf("outside by |T|: 0:%lld 1:%lld 2:%lld 3:%lld skipped>3:%lld\n",
           counts[0],counts[1],counts[2],counts[3],(long long)(N5-367-counts[0]-counts[1]-counts[2]-counts[3]));
    long long triples=0, checked=0;
    for(size_t i=0;i<S.size();i++)for(size_t j=i+1;j<S.size();j++)for(size_t k=j+1;k<S.size();k++){
        int s0=S[i],s1=S[j],s2=S[k];
        // all subsets of {s0,s1,s2}
        int elems[3]={s0,s1,s2};
        vector<int> cand;
        for(int mask=0;mask<8;mask++){
            int t[3]={-1,-1,-1}; int m=0;
            for(int b=0;b<3;b++) if(mask&(1<<b)) t[m++]=elems[b];
            auto it=bucket.find(key3(t[0],t[1],t[2]));
            if(it!=bucket.end()) cand.insert(cand.end(),it->second.begin(),it->second.end());
        }
        triples++;
        if(cand.size()<4) continue;
        checked++;
        // brute force independent 4-set
        int m=cand.size();
        for(int x=0;x<m;x++)for(int y=x+1;y<m;y++){
            if(adj(cand[x],cand[y]))continue;
            for(int z=y+1;z<m;z++){
                if(adj(cand[x],cand[z])||adj(cand[y],cand[z]))continue;
                for(int w=z+1;w<m;w++){
                    if(adj(cand[x],cand[w])||adj(cand[y],cand[w])||adj(cand[z],cand[w]))continue;
                    printf("FOUND A: %d %d %d %d  with R={%d,%d,%d}\n",cand[x],cand[y],cand[z],cand[w],s0,s1,s2);
                    printf("A:");
                    for(int q:{cand[x],cand[y],cand[z],cand[w]}){
                        printf(" [");
                        for(int d=0;d<5;d++)printf("%d%s",dig[q][d],d<4?" ":"");
                        printf("]");
                    }
                    printf("\n");
                    return 0;
                }
            }
        }
    }
    printf("NO 3-EXCHANGE: triples=%lld nonempty_checked=%lld\n",triples,checked);
    return 0;
}
