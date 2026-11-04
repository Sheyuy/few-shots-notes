// xk8: connected k-exchange enumeration, optimised.
//  * T(v) as a bitset over S          -> touch/union in O(words)
//  * candidate adjacency as a bitset  -> conflict in O(1)
//  * only ACTIVE candidates (those sharing a blocker with the current set) are scanned
// Same canonical-order rule as xk7: extend by the smallest allowed adjacent index,
// forbidding all smaller adjacent candidates in that branch.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
using namespace std;
static const int N5=16807;
static int K,m,n,NW,AW;
static vector<uint64_t> M;        // m*NW  masks of T(v) over S
static vector<uint64_t> ADJ;      // m*AW  adjacency bitset
static vector<vector<int>> TL;    // T lists (S indices)
static vector<vector<int>> BYT;   // candidates per blocker
static vector<vector<char>> gconf;static bool selftest=false;
static int dig[N5][5];
static inline bool adj5(int a,int b){
    bool any=false;
    for(int i=0;i<5;i++){int d=dig[a][i]-dig[b][i];if(d<0)d=-d;if(d==6)d=1;if(d>1)return false;if(d==1)any=true;}
    return any;
}
static inline bool conflict(int a,int b){
    if(selftest) return (bool)gconf[a][b];
    return (ADJ[(size_t)a*AW+(b>>6)]>>(b&63))&1ULL;
}
static inline bool touchM(const uint64_t*a,const uint64_t*b){
    for(int i=0;i<NW;i++)if(a[i]&b[i])return true;return false;
}
static inline int uniSize(const uint64_t*U,const uint64_t*a){
    int c=0;for(int i=0;i<NW;i++)c+=__builtin_popcountll(U[i]|a[i]);return c;
}
struct Ctx{
    vector<uint8_t> Ucnt; vector<int> adjCnt,touchCnt,pos,active,A,cand,forLog; int popU=0;
    vector<char> inA,forbidden; vector<uint64_t> U;
    long long cnt=0;
    Ctx():Ucnt(n,0),adjCnt(m,0),touchCnt(m,0),pos(m,-1),A(),cand(),forLog(),
          inA(m,0),forbidden(m,0),U(NW,0),cnt(0){A.reserve(K+2);cand.reserve(4096);}
    inline void act_add(int w){ if(pos[w]<0){pos[w]=(int)active.size();active.push_back(w);} }
    inline void act_del(int w){ int p=pos[w]; if(p<0)return; int last=active.back();
        active[p]=last;pos[last]=p;active.pop_back();pos[w]=-1; }
    void setU(int s,int delta){
        if(Ucnt[s]==0&&delta>0){ Ucnt[s]=1;
            for(int w:BYT[s]){ touchCnt[w]++; if(++adjCnt[w]==1) act_add(w); } }
        else if(Ucnt[s]==1&&delta<0){ Ucnt[s]=0;
            for(int w:BYT[s]){ touchCnt[w]--; if(--adjCnt[w]==0) act_del(w); } }
        else Ucnt[s]=(uint8_t)(Ucnt[s]+delta);
    }
    void addA(int v){
        inA[v]=1;A.push_back(v);
        popU+=(int)TL[v].size()-touchCnt[v];
        for(int s:TL[v])setU(s,+1);
    }
    void delA(int v){
        A.pop_back();inA[v]=0;
        for(int s:TL[v])setU(s,-1);
        popU-=(int)TL[v].size()-touchCnt[v];
    }
    void rec(int root){
        if((int)A.size()==K+1){cnt++;return;}
        vector<int> cand;cand.reserve(4096);
        for(int v:active){
            if(v<=root)continue;
            if(inA[v]||forbidden[v])continue;
            if(touchCnt[v]<=0)continue;
            bool bad=false;
            for(int a:A)if(conflict(a,v)){bad=true;break;}
            if(bad)continue;
            if(popU+(int)TL[v].size()-touchCnt[v]>K)continue;
            cand.push_back(v);
        }
        sort(cand.begin(),cand.end());
        for(size_t i=0;i<cand.size();i++){
            int v=cand[i];
            size_t mark=forLog.size();
            for(size_t j=0;j<i;j++){forbidden[cand[j]]=1;forLog.push_back(cand[j]);}
            addA(v);
            rec(root);
            delA(v);
            for(size_t j=mark;j<forLog.size();j++)forbidden[forLog[j]]=0;
            forLog.resize(mark);
        }
    }
    void doRoot(int root){
        // active set for an empty A is empty; seed with neighbours of root
        A.clear();popU=0;
        addA(root);
        rec(root);
        delA(root);
    }
};
long long runMT(int threads){
    vector<Ctx> ctx(threads);
    atomic<int> nxt(0); atomic<long long> total(0);
    vector<thread> th;
    for(int t=0;t<threads;t++){
        th.emplace_back([&,t]{
            Ctx&c=ctx[t];
            while(true){ int r=nxt.fetch_add(1); if(r>=m)break; c.doRoot(r); }
            total.fetch_add(c.cnt);
        });
    }
    for(auto&x:th)x.join();
    return total.load();
}
long long brute(){
    long long c=0;vector<int> idx(K+1);
    for(int i=0;i<=K;i++)idx[i]=i;
    while(true){
        vector<int> UU;bool ok=true;
        for(int q=0;q<=K&&ok;q++){for(int s:TL[idx[q]])UU.push_back(s);
            sort(UU.begin(),UU.end());UU.erase(unique(UU.begin(),UU.end()),UU.end());
            if((int)UU.size()>K)ok=false;}
        for(int i=0;i<=K&&ok;i++)for(int j=i+1;j<=K;j++)if(conflict(idx[i],idx[j])){ok=false;break;}
        if(ok){
            vector<char> vis(K+1,0);vector<int> st{0};vis[0]=1;int seen=1;
            while(!st.empty()){int x=st.back();st.pop_back();
                for(int y=0;y<=K;y++)if(!vis[y]){
                    bool t=false;for(int a:TL[idx[x]])for(int b:TL[idx[y]])if(a==b){t=true;break;}
                    if(t){vis[y]=1;seen++;st.push_back(y);}}}
            if(seen==K+1)c++;
        }
        int p=K;while(p>=0&&idx[p]==m-1-(K-p))p--;
        if(p<0)break;idx[p]++;for(int q=p+1;q<=K;q++)idx[q]=idx[q-1]+1;
    }
    return c;
}
static void buildStructs(){
    NW=(n+63)/64; AW=(m+63)/64;
    M.assign((size_t)m*NW,0);
    BYT.assign(n,{});
    for(int v=0;v<m;v++){
        for(int s:TL[v]){ M[(size_t)v*NW+(s>>6)] |= 1ULL<<(s&63); BYT[s].push_back(v); }
    }
    if(!selftest){
        ADJ.assign((size_t)m*AW,0);
        for(int i=0;i<m;i++)for(int j=i+1;j<m;j++){
            bool a=adj5(i<m?0:0,0);(void)a;
        }
    }
}
int main(int argc,char**argv){
    selftest=argc>1&&string(argv[1])=="--selftest";
    if(selftest){
        int N=argc>2?atoi(argv[2]):2000,mmin=argc>3?atoi(argv[3]):8,mrg=argc>4?atoi(argv[4]):7,kmax=argc>5?atoi(argv[5]):6;
        printf("SELFTEST instances=%d m=%d+[0,%d) K=1..%d\n",N,mmin,mrg,kmax);fflush(stdout);
        srand(20261103);long long bad=0,pos_=0,tot=0,mx=0;
        for(int it=0;it<N;it++){
            m=mmin+rand()%mrg;K=1+rand()%kmax;int ns=3+rand()%5;n=ns;
            TL.assign(m,{});
            for(int v=0;v<m;v++){vector<int> t;
                for(int s=0;s<ns;s++)if(rand()%3==0)t.push_back(s);
                if(t.empty())t.push_back(rand()%ns);
                sort(t.begin(),t.end());t.erase(unique(t.begin(),t.end()),t.end());TL[v]=t;}
            gconf.assign(m,vector<char>(m,0));
            for(int i=0;i<m;i++)for(int j=i+1;j<m;j++)if(rand()%4==0)gconf[i][j]=gconf[j][i]=1;
            buildStructs();
            long long b=brute();
            long long s1=runMT(1);
            long long s4=runMT(4);
            if(b>0){pos_++;tot+=b;if(b>mx)mx=b;}
            if(b!=s1||b!=s4){bad++;if(bad<4)printf("MISMATCH it=%d m=%d K=%d brute=%lld mt1=%lld mt4=%lld\n",it,m,K,b,s1,s4);}
        }
        printf("SELFTEST instances=%d mismatches=%lld positive=%lld total=%lld max=%lld\n",N,bad,pos_,tot,mx);
        return bad?1:0;
    }
    K=argc>1?atoi(argv[1]):9;int threads=argc>2?atoi(argv[2]):8;
    const char* path=argc>3?argv[3]:"R367.txt";
    for(int v=0;v<N5;v++){int x=v;for(int i=0;i<5;i++){dig[v][i]=x%7;x/=7;}}
    FILE* f=fopen(path,"r");if(!f){printf("NO_FILE %s\n",path);return 1;}
    vector<int> S;int a[5];
    while(fscanf(f,"%d %d %d %d %d",a,a+1,a+2,a+3,a+4)==5){int x=0;for(int i=4;i>=0;i--)x=x*7+a[i];S.push_back(x);}
    fclose(f);
    n=(int)S.size();
    vector<char> inS(N5,0);for(int s:S)inS[s]=1;
    vector<int> V;TL.clear();
    for(int v=0;v<N5;v++){if(inS[v])continue;vector<int> t;
        for(int i=0;i<n;i++)if(adj5(v,S[i]))t.push_back(i);
        if((int)t.size()<=K){TL.push_back(t);V.push_back(v);m=TL.size();}}
    m=(int)TL.size();
    printf("K=%d n=%d m=%d threads=%d connected=1 fast=1\n",K,n,m,threads);fflush(stdout);
    buildStructs();
    if(!selftest){
        ADJ.assign((size_t)m*AW,0);
        auto t0=chrono::steady_clock::now();
        for(int i=0;i<m;i++)for(int j=i+1;j<m;j++){
            if(adj5(V[i],V[j])){ADJ[(size_t)i*AW+(j>>6)]|=1ULL<<(j&63);ADJ[(size_t)j*AW+(i>>6)]|=1ULL<<(i&63);}
        }
        printf("ADJ built in %.1f s\n",chrono::duration<double>(chrono::steady_clock::now()-t0).count());fflush(stdout);
    }
    auto t0=chrono::steady_clock::now();
    long long c=runMT(threads);
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("CONNECTED_SETS size=%d union<=%d count=%lld seconds=%.1f\n",K+1,K,c,sec);
    return 0;
}
