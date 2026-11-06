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
def w1(a,b): return all(min((x-y)%7,(y-x)%7)<=1 for x,y in zip(a,b))
def tr(w): return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])
PAIRS8=[((1,3,4,4,6),(2,3,5,4,6)),((3,4,0,3,5),(2,4,6,3,5)),((5,3,1,3,4),(5,3,2,3,5)),((4,4,6,1,6),(5,4,6,0,6)),((6,0,6,4,5),(6,1,6,5,5)),((0,3,5,6,5),(6,3,5,0,5)),((6,4,3,4,0),(6,4,2,4,6)),((6,4,5,3,2),(6,5,5,3,1))]
X0=[tr(w) for w in Rlist]; X0[267]=(1,5,6,3,5)
N=[0]*16807
for s in range(16807):
    d=dec(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        m|=1<<enc([(d[i]+off[i])%7 for i in range(5)])
    N[s]=m
class TwoSAT:
    def __init__(self,n): self.n=n; self.g=[[] for _ in range(2*n)]
    def add(self,l1,l2): self.g[l1^1].append(l2); self.g[l2^1].append(l1)
    def forbid(self,v1,a1,v2,a2): self.add(2*v1+(0 if a1 else 1), 2*v2+(0 if a2 else 1))
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
                else: order.append(v); st.pop()
        rg=[[] for _ in range(2*n)]
        for v in range(2*n):
            for w in g[v]: rg[w].append(v)
        comp=[-1]*(2*n); c=0
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
        return [comp[2*v]<comp[2*v+1] for v in range(n)]
def solve_pairs(pairs,X):
    d=len(pairs); nx=len(X); nv=d*nx*2
    ts=TwoSAT(nv)
    Xpos={enc(x):i for i,x in enumerate(X)}; Xset={enc(x) for x in X}
    def H(j,i): return j*nx+i
    def V(j,i): return d*nx+j*nx+i
    for i,x in enumerate(X):
        for j in range(d):
            for aj,a in ((0,pairs[j][0]),(1,pairs[j][1])):
                for k in range(d):
                    for ak,b in ((0,pairs[k][0]),(1,pairs[k][1])):
                        if not w1(a,b): continue
                        if j==k and aj==ak: continue
                        # (a,x) vs (b,x): adjacent iff a!=b
                        if a!=b: ts.forbid(H(j,i),aj,H(k,i),ak)
                        if a!=b: ts.forbid(V(j,i),aj,V(k,i),ak)
                        # (a,x) vs (x,b)
                        if w1(a,x) and w1(x,b) and not (a==x and b==x):
                            ts.forbid(H(j,i),aj,V(k,i),ak)
        for j in range(d):
            for aj,a in ((0,pairs[j][0]),(1,pairs[j][1])):
                if not w1(a,x): continue
                for ak,b in ((0,pairs[j][0]),(1,pairs[j][1])):
                    if not w1(x,b): continue
                    if a==x and b==x: continue
                    ts.forbid(H(j,i),aj,V(j,i),ak)
    for i,x in enumerate(X):
        for j in range(d):
            for aj,a in ((0,pairs[j][0]),(1,pairs[j][1])):
                ai=enc(a); mm=N[ai]
                while mm:
                    yy=(mm&-mm).bit_length()-1; mm&=mm-1
                    if yy not in Xset: continue
                    iy=Xpos[yy]
                    # (a,x) vs (y,b): w1(a,y) and w1(x,b); x!=y ensures distinct
                    if iy==i: continue
                    for k in range(d):
                        for ak,b in ((0,pairs[k][0]),(1,pairs[k][1])):
                            if w1(x,b):
                                ts.forbid(H(j,i),aj,V(k,iy),ak)
        for j in range(d):
            for aj,b in ((0,pairs[j][0]),(1,pairs[j][1])):
                bi=enc(b); mm=N[bi]
                while mm:
                    yy=(mm&-mm).bit_length()-1; mm&=mm-1
                    if yy not in Xset: continue
                    iy=Xpos[yy]
                    if iy==i: continue
                    for k in range(d):
                        for ak,a in ((0,pairs[k][0]),(1,pairs[k][1])):
                            if w1(x,a):
                                ts.forbid(V(j,i),aj,H(k,iy),ak)
    return ts
def build_set(pairs,X,sol):
    d=len(pairs); nx=len(X)
    def H(j,i): return j*nx+i
    def V(j,i): return d*nx+j*nx+i
    deleted={enc(r) for r,q in pairs}
    R=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]
    if any(pairs[j][0] not in set(R) for j in range(d)):
        R=None
    return sol
def check(P):
    T={}
    for x in P:
        p=enc(x[:5]); s=enc(x[5:]); T.setdefault(p,set()).add(s)
    U={}
    for p,ss in T.items():
        u=0
        for s in ss: u|=N[s]
        U[p]=u
    for p,ss in T.items():
        tb=0
        for s in ss: tb|=1<<s
        for s in ss:
            if (tb & N[s]) != (1<<s): return False
        m=N[p]
        while m:
            b=(m&-m).bit_length()-1; m&=m-1
            if b==p: continue
            if b in T and (tb & U[b]): return False
    return True
def build_from_sol(pairs,Rlist,X,sol,baseR):
    d=len(pairs); nx=len(X)
    def H(j,i): return j*nx+i
    def V(j,i): return d*nx+j*nx+i
    deleted={enc(r) for r,q in pairs}
    B=[w for w in baseR if enc(w) not in deleted]
    C=set()
    for b1 in B:
        for b2 in B: C.add(b1+b2)
    for i,x in enumerate(X):
        for j in range(d):
            h=pairs[j][0] if not sol[H(j,i)] else pairs[j][1]
            v=pairs[j][0] if not sol[V(j,i)] else pairs[j][1]
            C.add(h+x); C.add(x+v)
    return C
if __name__=='__main__':
    X=X0
    Rbase=Rlist
    ts=solve_pairs(PAIRS8,X); sol=ts.solve()
    print('8-pair SAT',sol is not None,flush=True)
    if sol:
        C=build_from_sol(PAIRS8,Rbase,X,sol,Rbase)
        print('8 size',len(C),'valid',check(C),flush=True)
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
    print('9 pairs',len(pairs),flush=True)
    ts9=solve_pairs(pairs,X); sol9=ts9.solve()
    print('9-pair SAT',sol9 is not None,flush=True)
    if sol9:
        C=build_from_sol(pairs,R1,X,sol9,R1)
        print('9 size',len(C),'valid',check(C),flush=True)
