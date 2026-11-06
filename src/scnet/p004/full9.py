import itertools, sys
Rlist=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]
def enc(d):
    r=0
    for v in d: r=r*7+v
    return r
def dec(i):
    d=[0]*5
    for k in range(4,-1,-1): d[k]=i%7; i//=7
    return tuple(d)
def adj5(a,b): return a!=b and all(min((x-y)%7,(y-x)%7)<=1 for x,y in zip(a,b))
def conf(x,P): return any(all(min((u-v)%7,(v-u)%7)<=1 for u,v in zip(x,y)) for y in P)
N=[0]*16807
for s in range(16807):
    d=dec(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        m|=1<<enc([(d[i]+off[i])%7 for i in range(5)])
    N[s]=m
def tr(w): return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])
PAIRS8=[((1,3,4,4,6),(2,3,5,4,6)),((3,4,0,3,5),(2,4,6,3,5)),((5,3,1,3,4),(5,3,2,3,5)),((4,4,6,1,6),(5,4,6,0,6)),((6,0,6,4,5),(6,1,6,5,5)),((0,3,5,6,5),(6,3,5,0,5)),((6,4,3,4,0),(6,4,2,4,6)),((6,4,5,3,2),(6,5,5,3,1))]
X0=[tr(w) for w in Rlist]; X0[267]=(1,5,6,3,5)
class TwoSAT:
    def __init__(self,n):
        self.n=n; self.g=[[] for _ in range(2*n)]; self.unsat=False
    def add(self,l1,l2):
        self.g[l1^1].append(l2); self.g[l2^1].append(l1)
    def forbid(self,v1,a1,v2,a2):
        self.add(2*v1+(0 if a1 else 1), 2*v2+(0 if a2 else 1))
    def solve(self):
        n=self.n; g=self.g
        seen=[False]*(2*n); order=[]
        for s in range(2*n):
            if seen[s]: continue
            st=[(s,0)]; seen[s]=True
            while st:
                v,i=st[-1]
                if i<len(g[v]):
                    w=g[v][i]; st[-1]=(v,i+1)
                    if not seen[w]: seen[w]=True; st.append((w,0))
                else:
                    order.append(v); st.pop()
        comp=[-1]*(2*n); c=0
        rg=[[] for _ in range(2*n)]
        for v in range(2*n):
            for w in g[v]: rg[w].append(v)
        for s in reversed(order):
            if comp[s]!=-1: continue
            st=[s]; comp[s]=c
            while st:
                v=st.pop()
                for w in rg[v]:
                    if comp[w]==-1: comp[w]=c; st.append(w)
            c+=1
        for v in range(n):
            if comp[2*v]==comp[2*v+1]: return None
        return [comp[2*v]>comp[2*v+1] for v in range(n)]
def build_ts(pairs,X):
    d=len(pairs); nv=d*len(X)
    ts=TwoSAT(2*nv)
    def H(j,i): return j*len(X)+i
    def V(j,i): return d*len(X)+j*len(X)+i
    # within x: h_j vs h_k, h_j vs v_k
    for i,x in enumerate(X):
        for j in range(d):
            for k in range(d):
                for aj,a in ((0,pairs[j][0]),(1,pairs[j][1])):
                    for ak,b in ((0,pairs[k][0]),(1,pairs[k][1])):
                        if j==k and aj==ak: continue
                        if adj5(a,b):
                            ts.forbid(H(j,i),aj,H(k,i),ak)
                        if adj5(a,b):
                            ts.forbid(H(j,i),aj,V(k,i),ak)
        # h_j vs v_j same x: a~x and x~b
        for j in range(d):
            for aj,a in ((0,pairs[j][0]),(1,pairs[j][1])):
                if not adj5(a,x): continue
                for ak,b in ((0,pairs[j][0]),(1,pairs[j][1])):
                    if adj5(x,b):
                        ts.forbid(H(j,i),aj,V(j,i),ak)
    # cross: h_j(x) vs v_k(y)
    Xidx=[enc(x) for x in X]
    for i,x in enumerate(X):
        xi=enc(x)
        for j in range(d):
            for aj,a in ((0,pairs[j][0]),(1,pairs[j][1])):
                ai=enc(a); mm=N[ai]
                while mm:
                    yy=(mm&-mm).bit_length()-1; mm&=mm-1
                    if yy not in Xset: continue
                    y=dec(yy); iy=Xpos[yy]
                    for k in range(d):
                        for ak,b in ((0,pairs[k][0]),(1,pairs[k][1])):
                            if adj5(x,b):
                                ts.forbid(H(j,i),aj,V(k,iy),ak)
        for j in range(d):
            for aj,b in ((0,pairs[j][0]),(1,pairs[j][1])):
                bi=enc(b); mm=N[bi]
                while mm:
                    yy=(mm&-mm).bit_length()-1; mm&=mm-1
                    if yy not in Xset: continue
                    y=dec(yy); iy=Xpos[yy]
                    for k in range(d):
                        for ak,a in ((0,pairs[k][0]),(1,pairs[k][1])):
                            if adj5(x,a):
                                ts.forbid(V(j,i),aj,H(k,iy),ak)
    return ts
if __name__=='__main__':
    X=X0; Xset=set(X); Xpos={enc(x):i for i,x in enumerate(X)}
    sys.setrecursionlimit(10000)
    # baseline 8 pairs sanity
    ts=build_ts(PAIRS8,X)
    sol=ts.solve()
    print('8-pair SAT?',sol is not None,flush=True)
    # 9 pairs from R1
    old=(6,0,6,4,5); new=(6,1,6,5,5)
    R1=[new if w==old else w for w in Rlist]
    Rm=0
    for w in R1: Rm|=1<<enc(w)
    sg=[]
    for s in range(16807):
        if (Rm>>s)&1: continue
        b=N[s]&Rm
        if b.bit_count()==1: sg.append((s,(b&-b).bit_length()-1))
    pairs=[(dec(r),dec(q)) for q,r in sg]
    print('pairs',len(pairs),flush=True)
    ts9=build_ts(pairs,X)
    sol9=ts9.solve()
    print('9-pair SAT?',sol9 is not None,flush=True)
