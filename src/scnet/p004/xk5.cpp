// Connected (blocker-hypergraph) exchange enumeration.
// Any *minimal* improving exchange has a connected bipartite (A, T(A)) graph, and
// every k' < K is already excluded, so at level K it is enough to enumerate
// connected A.  Sets are grown from their smallest-index vertex (ESU style) with a
// per-level "banned" list so each connected set is produced once.
//   ./xk5 K [threads]           real instance R367
//   ./xk5 --selftest            brute force vs ESU on random small instances
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>
using namespace std;
static const int N5=16807;
int K, m;
vector<vector<int>> T;          // T(v), sorted
vector<vector<int>> adjS;       // for each S point: external points adjacent to it
vector<int> Slist;
int dig[N5][5];
static inline bool adj(int a,int b){
    bool any=false;
    for(int i=0;i<5;i++){int d=dig[a][i]-dig[b][i];if(d<0)d=-d;if(d==6)d=1;if(d>1)return false;if(d==1)any=true;}
    return any;
}
// ---------- generic small-instance machinery (selftest) ----------
vector<vector<char>> gconf;     // conflict matrix
bool selftest=false;
static inline bool conflict(int a,int b){ return selftest? (bool)gconf[a][b] : adj(a,b); }
static inline bool touch(int a,int b){
    const auto&x=T[a];const auto&y=T[b];size_t i=0,j=0;
    while(i<x.size()&&j<y.size()){ if(x[i]==y[j])return true; if(x[i]<y[j])i++; else j++; }
    return false;
}
static void mergeInto(vector<int>&U,const vector<int>&t){
    vector<int> o;o.reserve(U.size()+t.size());size_t i=0,j=0;
    while(i<U.size()||j<t.size()){ if(j>=t.size()||(i<U.size()&&U[i]<t[j]))o.push_back(U[i++]);
        else if(i>=U.size()||t[j]<U[i])o.push_back(t[j++]); else {o.push_back(U[i++]);j++;} }
    U.swap(o);
}
static int unionSize(const vector<int>&U,const vector<int>&t){
    size_t i=0,j=0;int n=0;
    while(i<U.size()||j<t.size()){ if(j>=t.size()||(i<U.size()&&U[i]<t[j])){i++;} else if(i>=U.size()||t[j]<U[i]){j++;}
        else {i++;j++;} n++; }
    return n;
}
long long cnt=0; bool found=false; bool stopAtFirst=true;
vector<int> A, U;
vector<char> banned;
vector<int> banLog;
int root;
static inline bool inA(int v){ return find(A.begin(),A.end(),v)!=A.end(); }
void rec(vector<int>&ext){
    if(found)return;
    if((int)A.size()==K+1){cnt++;if(stopAtFirst)found=true;return;}
    size_t markStart=banLog.size();
    while(!ext.empty()&&!found){
        int w=ext.back();ext.pop_back();
        banned[w]=1; banLog.push_back(w);
        vector<int> U2=U;mergeInto(U2,T[w]);
        A.push_back(w);
        vector<int> ext2=ext;
        // candidate extensions: points touching w (H-neighbours), index>root
        for(int u=w+1;u<m&&!found;u++){
            if(inA(u))continue;
            if(banned[u])continue;
            if(find(ext2.begin(),ext2.end(),u)!=ext2.end())continue;
            if(!touch(u,w))continue;
            bool bad=false;for(int q:A)if(conflict(q,u)){bad=true;break;}
            if(bad)continue;
            if(unionSize(U2,T[u])>K)continue;
            ext2.push_back(u);
        }
        vector<int> saveU=U;U=U2;
        rec(ext2);
        U=saveU;A.pop_back();
    }
    for(size_t i=markStart;i<banLog.size();i++)banned[banLog[i]]=0;
    banLog.resize(markStart);
}
long long run_esu(){
    cnt=0;found=false;banned.assign(m,0);banLog.clear();
    for(root=0;root<m&&!found;root++){
        A.clear();A.push_back(root);U=T[root];
        vector<int> ext;
        for(int u=root+1;u<m;u++){
            if(!touch(u,root))continue;
            if(conflict(root,u))continue;
            if(unionSize(U,T[u])>K)continue;
            ext.push_back(u);
        }
        rec(ext);
    }
    return cnt;
}
// ---------- brute force for selftest ----------
long long brute(){
    long long c=0;
    vector<int> idx(K+1);
    for(int i=0;i<=K;i++)idx[i]=i;
    while(true){
        vector<int> UU;
        bool ok=true;
        for(int q=0;q<=K&&ok;q++){ mergeInto(UU,T[idx[q]]); if((int)UU.size()>K)ok=false; }
        for(int i=0;i<=K&&ok;i++)for(int j=i+1;j<=K;j++)if(conflict(idx[i],idx[j])){ok=false;break;}
        if(ok){ // connectivity
            vector<char> vis(K+1,0);vector<int> st{0};vis[0]=1;int seen=1;
            while(!st.empty()){int x=st.back();st.pop_back();
                for(int y=0;y<=K;y++)if(!vis[y]&&touch(idx[x],idx[y])){vis[y]=1;seen++;st.push_back(y);} }
            if(seen==K+1)c++;
        }
        int p=K;while(p>=0&&idx[p]==m-1-(K-p))p--;
        if(p<0)break;idx[p]++;for(int q=p+1;q<=K;q++)idx[q]=idx[q-1]+1;
    }
    return c;
}
int main(int argc,char**argv){
    selftest = argc>1 && string(argv[1])=="--selftest";
    if(selftest){
        srand(12345); long long bad=0;
        for(int it=0;it<3000;it++){
            m=7+rand()%6; K=1+rand()%3;
            int ns=3+rand()%4;
            T.assign(m,{});
            for(int v=0;v<m;v++){
                vector<int> t;
                for(int s=0;s<ns;s++)if(rand()%3==0)t.push_back(s);
                if(t.empty())t.push_back(rand()%ns);
                sort(t.begin(),t.end());t.erase(unique(t.begin(),t.end()),t.end());
                T[v]=t;
            }
            gconf.assign(m,vector<char>(m,0));
            for(int i=0;i<m;i++)for(int j=i+1;j<m;j++)if(rand()%4==0)gconf[i][j]=gconf[j][i]=1;
            stopAtFirst=false; long long b=brute(); long long e=run_esu();
            if(b!=e){bad++;if(bad<5)printf("MISMATCH it=%d m=%d K=%d brute=%lld esu=%lld\n",it,m,K,b,e);}
        }
        printf("SELFTEST instances=3000 mismatches=%lld\n",bad);
        return bad?1:0;
    }
    K=argc>1?atoi(argv[1]):9;
    int threads=argc>2?atoi(argv[2]):8;
    const char* path="/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt";
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r");if(!f)return 1;
    vector<int> S;int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5){int x=0;for(int i=4;i>=0;i--)x=x*7+a[i];S.push_back(x);}
    fclose(f);
    vector<char> inS(N5,0);for(int s:S)inS[s]=1;
    vector<int> extPts;
    for(int v=0;v<N5;v++){
        if(inS[v])continue;
        vector<int> t;
        for(int s:S)if(adj(v,s))t.push_back(s);
        if((int)t.size()<=K){extPts.push_back(v);T.push_back(t);}
    }
    m=extPts.size();
    printf("K=%d m=%d\n",K,m);fflush(stdout);
    auto t0=chrono::steady_clock::now();
    long long c=run_esu();
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("CONNECTED_SETS size=%d union<=%d count=%lld found=%d seconds=%.1f\n",K+1,K,c,(int)found,sec);
    return 0;
}
