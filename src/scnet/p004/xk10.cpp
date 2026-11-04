// xk10: connected k-exchange enumeration, incremental counters.
//  vs xk9: the per-node scan over "active" candidates is replaced by a maintained
//  bitset freeB = { v : touchCnt[v]>0 && conflictCnt[v]==0 && !inA[v] }.
//  conflictCnt  : number of a in A adjacent to v        (updated over candidate-CSR)
//  touchCnt     : |T(v) ∩ T(A)|                        (updated over blocker lists)
//  union size   : popU + |T(v)| - touchCnt[v]          (O(1))
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
static int K,m,n,NW,AW,W;
static vector<uint64_t> M;              // m*NW  T(v) as bitset over S
static vector<uint64_t> ADJ;            // m*AW  adjacency (real mode)
static vector<vector<int>> TL,BYT;
static vector<vector<char>> gconf;
static vector<int> coff; static vector<int> cadj;   // candidate graph CSR
static bool selftest=false;
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
struct Ctx{
    vector<uint8_t> Ucnt;
    vector<int16_t> touchCnt,conflictCnt;
    vector<char> inA,forbidden;
    vector<uint64_t> freeB;
    vector<int> A; int popU=0;
    long long cnt=0,nodes=0,scans=0;
    Ctx():Ucnt(n,0),touchCnt(m,0),conflictCnt(m,0),inA(m,0),forbidden(m,0),
          freeB(W,0),A(),cnt(0),nodes(0),scans(0){A.reserve(K+2);}
    inline void setbit(int v){ freeB[v>>6]|=1ULL<<(v&63); }
    inline void clrbit(int v){ freeB[v>>6]&=~(1ULL<<(v&63)); }
    void addA(int v){
        popU+=(int)TL[v].size()-touchCnt[v];
        for(int s:TL[v]){
            if(Ucnt[s]==0){ Ucnt[s]=1;
                for(int w:BYT[s]){ touchCnt[w]++;
                    if(touchCnt[w]==1&&conflictCnt[w]==0&&!inA[w])setbit(w); } }
            else Ucnt[s]=(uint8_t)(Ucnt[s]+1);
        }
        for(int i=coff[v];i<coff[v+1];i++){ int w=cadj[i];
            if(conflictCnt[w]==0)clrbit(w);
            conflictCnt[w]++; }
        inA[v]=1;clrbit(v);A.push_back(v);
    }
    void delA(int v){
        for(int i=coff[v];i<coff[v+1];i++){ int w=cadj[i];
            conflictCnt[w]--;
            if(conflictCnt[w]==0&&touchCnt[w]>0&&!inA[w])setbit(w); }
        for(int s:TL[v]){
            Ucnt[s]=(uint8_t)(Ucnt[s]-1);
            if(Ucnt[s]==0){ for(int w:BYT[s]){ touchCnt[w]--;
                    if(touchCnt[w]==0)clrbit(w); } }
            else { }
        }
        inA[v]=0;A.pop_back();
        if(touchCnt[v]>0&&conflictCnt[v]==0)setbit(v);
        popU-=(int)TL[v].size()-touchCnt[v];
    }
    void rec(int root){
        nodes++;
        if((int)A.size()==K+1){cnt++;return;}
        vector<int> cand; cand.reserve(4096);
        for(int wi=0;wi<W;wi++){
            uint64_t b=freeB[wi];
            while(b){
                int v=(wi<<6)+__builtin_ctzll(b); b&=b-1;
                scans++;
                if(v<=root||inA[v]||forbidden[v])continue;
                if(popU+(int)TL[v].size()-touchCnt[v]>K)continue;
                cand.push_back(v);
            }
        }
        for(size_t i=0;i<cand.size();i++){
            int v=cand[i];
            for(size_t j=0;j<i;j++)forbidden[cand[j]]=1;
            addA(v);
            rec(root);
            delA(v);
            for(size_t j=0;j<i;j++)forbidden[cand[j]]=0;
        }
    }
    void doRoot(int root){ A.clear();popU=0; addA(root); rec(root); delA(root); }
};
static atomic<long long> g_nodes(0),g_scans(0);
long long runMT(int threads){
    vector<Ctx> ctx(threads); atomic<int> nxt(0); atomic<long long> total(0);
    vector<thread> th;
    for(int t=0;t<threads;t++) th.emplace_back([&,t]{
        Ctx&c=ctx[t];
        while(true){ int r=nxt.fetch_add(1); if(r>=m)break; c.doRoot(r); }
        total.fetch_add(c.cnt);g_nodes+=c.nodes;g_scans+=c.scans;
    });
    for(auto&x:th)x.join();
    return total.load();
}
long long bruteC(int K_,int m_){
    long long c=0;vector<int> idx(K_+1);
    for(int i=0;i<=K_;i++)idx[i]=i;
    while(true){
        vector<int> UU;bool ok=true;
        for(int q=0;q<=K_&&ok;q++){for(int s:TL[idx[q]])UU.push_back(s);
            sort(UU.begin(),UU.end());UU.erase(unique(UU.begin(),UU.end()),UU.end());
            if((int)UU.size()>K_)ok=false;}
        for(int i=0;i<=K_&&ok;i++)for(int j=i+1;j<=K_;j++)if(conflict(idx[i],idx[j])){ok=false;break;}
        if(ok){
            vector<char> vis(K_+1,0);vector<int> st{0};vis[0]=1;int seen=1;
            while(!st.empty()){int x=st.back();st.pop_back();
                for(int y=0;y<=K_;y++)if(!vis[y]){
                    bool t=false;for(int a:TL[idx[x]])for(int b:TL[idx[y]])if(a==b){t=true;break;}
                    if(t){vis[y]=1;seen++;st.push_back(y);}}}
            if(seen==K_+1)c++;
        }
        int p=K_;while(p>=0&&idx[p]==m_-1-(K_-p))p--;
        if(p<0)break;idx[p]++;for(int q=p+1;q<=K_;q++)idx[q]=idx[q-1]+1;
    }
    return c;
}
static void buildCSR(){
    coff.assign(m+1,0);
    vector<int> deg(m,0);
    for(int i=0;i<m;i++)for(int j=0;j<m;j++)if(i!=j&&conflict(i,j))deg[i]++;
    coff[0]=0;for(int i=0;i<m;i++)coff[i+1]=coff[i]+deg[i];
    cadj.assign(coff[m],0); vector<int> fill(coff.begin(),coff.end()-1);
    for(int i=0;i<m;i++)for(int j=0;j<m;j++)if(i!=j&&conflict(i,j))cadj[fill[i]++]=j;
}
int main(int argc,char**argv){
    selftest=argc>1&&string(argv[1])=="--selftest";
    if(selftest){
        int N=argc>2?atoi(argv[2]):2000,mmin=argc>3?atoi(argv[3]):8,mrg=argc>4?atoi(argv[4]):7,kmax=argc>5?atoi(argv[5]):6;
        printf("SELFTEST instances=%d m=%d+[0,%d) K=1..%d\n",N,mmin,mrg,kmax);fflush(stdout);
        srand(20261103);long long bad=0,pos_=0,tot=0,mx=0;
        for(int it=0;it<N;it++){
            m=mmin+rand()%mrg;K=1+rand()%kmax;int ns=3+rand()%5;n=ns;W=(m+63)/64;NW=(n+63)/64;AW=(m+63)/64;
            TL.assign(m,{});
            for(int v=0;v<m;v++){vector<int> t;
                for(int s=0;s<ns;s++)if(rand()%3==0)t.push_back(s);
                if(t.empty())t.push_back(rand()%ns);
                sort(t.begin(),t.end());t.erase(unique(t.begin(),t.end()),t.end());TL[v]=t;}
            gconf.assign(m,vector<char>(m,0));
            for(int i=0;i<m;i++)for(int j=i+1;j<m;j++)if(rand()%4==0)gconf[i][j]=gconf[j][i]=1;
            BYT.assign(n,{}); for(int v=0;v<m;v++)for(int s:TL[v])BYT[s].push_back(v);
            buildCSR();
            long long b=bruteC(K,m);
            long long s1=runMT(1),s4=runMT(4);
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
        if((int)t.size()<=K){TL.push_back(t);V.push_back(v);}}
    m=(int)TL.size();
    NW=(n+63)/64;AW=(m+63)/64;W=(m+63)/64;
    printf("K=%d n=%d m=%d threads=%d connected=1 fast=2\n",K,n,m,threads);fflush(stdout);
    BYT.assign(n,{}); for(int v=0;v<m;v++)for(int s:TL[v])BYT[s].push_back(v);
    AW=(m+63)/64;ADJ.assign((size_t)m*AW,0);
    auto t0=chrono::steady_clock::now();
    for(int i=0;i<m;i++)for(int j=i+1;j<m;j++)
        if(adj5(V[i],V[j])){ADJ[(size_t)i*AW+(j>>6)]|=1ULL<<(j&63);ADJ[(size_t)j*AW+(i>>6)]|=1ULL<<(i&63);}
    buildCSR();
    printf("ADJ+CSR built in %.1f s (%d edges)\n",chrono::duration<double>(chrono::steady_clock::now()-t0).count(),coff[m]/2);fflush(stdout);
    t0=chrono::steady_clock::now();
    long long c=runMT(threads);
    double sec=chrono::duration<double>(chrono::steady_clock::now()-t0).count();
    printf("CONNECTED_SETS size=%d union<=%d count=%lld seconds=%.1f\n",K+1,K,c,sec);
    printf("STATS nodes=%lld scans=%lld (%.1f scans/node)\n",(long long)g_nodes,(long long)g_scans,g_nodes?double(g_scans)/double(g_nodes):0.0);
    return 0;
}
