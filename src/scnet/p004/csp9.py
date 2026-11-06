import itertools, sys
Rlist=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]
R=set(Rlist)
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
D8={enc(r) for r,q in PAIRS8}; Q8={enc(q) for r,q in PAIRS8}
X0=[tr(w) for w in Rlist]; X0[267]=(1,5,6,3,5)
X=X0
Xidx=[enc(x) for x in X]
Xset=set(X)
J0={0,5,6}
PH=[PAIRS8[j][0] for j in J0]+[PAIRS8[j][1] for j in range(8) if j not in J0]
PV=[PAIRS8[j][1] for j in J0]+[PAIRS8[j][0] for j in range(8) if j not in J0]
A=set(x for x in X if conf(x,PV)); Ds=set(x for x in X if conf(x,PH))
def hval(x,j):
    inJ0=j in J0
    s=((inJ0 and x in A) or ((not inJ0) and x in Ds))
    return PAIRS8[j][s]
def vval(x,j):
    inJ0=j in J0
    s=((inJ0 and x in Ds) or ((not inJ0) and x in A))
    return PAIRS8[j][s]
# fixed groups F: B'xB' + original families
def fixed_groups(r8):
    T={}
    B=[w for w in Rlist if enc(w) not in D8 and w!=r8]
    for b1 in B:
        for b2 in B: T.setdefault(enc(b1),set()).add(enc(b2))
    for x in X:
        for j in range(8):
            h=hval(x,j); v=vval(x,j)
            T.setdefault(enc(h),set()).add(enc(x))
            T.setdefault(enc(x),set()).add(enc(v))
    return T
def groups_bits(T):
    return {p:sum(1<<s for s in ss) for p,ss in T.items()}
def solve(r8,q8,TB):
    # TB: dict p->bitset
    def Ba(a):
        m=0; ai=enc(a)
        mm=N[ai]
        while mm:
            b=(mm&-mm).bit_length()-1; mm&=mm-1
            m|=TB.get(b,0)
        return m
    Br=Ba(r8); Bq=Ba(q8)
    # 2-SAT: vars 0..366 H, 367..733 V ; lit: 2*v (false=r), 2*v+1 (true=q)
    nv=2*len(X); g=[[] for _ in range(2*nv)]
    def add_clause(l1,l2):
        g[l1^1].append(l2); g[l2^1].append(l1)
    def lit(v,val): return 2*v+(1 if val else 0)
    unsat=False
    for i,x in enumerate(X):
        for val,a in ((0,r8),(1,q8)):
            bm=Br if val==0 else Bq
            if bm & N[enc(x)]:
                add_clause(lit(i,val)^1, lit(i,val)^1)  # forbid val: clause (not lit)
    # same-variable conflicts (h(x),x) vs (x,v(x)): a~x and x~b
    for i,x in enumerate(X):
        xi=enc(x)
        for ha,a in ((0,r8),(1,q8)):
            if not adj5(a,x): continue
            for vb,b in ((0,r8),(1,q8)):
                if adj5(x,b):
                    add_clause(lit(i,ha)^1, lit(len(X)+i,vb)^1)
    # cross: (h(x),x) vs (y,v(y)): forbid H_x=a, V_y=b if a~y and x~b
    for i,x in enumerate(X):
        xi=enc(x)
        for ha,a in ((0,r8),(1,q8)):
            ai=enc(a); mm=N[ai]
            while mm:
                j=(mm&-mm).bit_length()-1; mm&=mm-1
                if j not in Xset: continue
                y=dec(j)
                for vb,b in ((0,r8),(1,q8)):
                    if adj5(x,b):
                        add_clause(lit(i,ha)^1, lit(len(X)+j,vb)^1)
    # cross symmetric: (x,v(x)) vs (h(y),y): forbid V_x=b, H_y=a if b~y and x~a
    for i,x in enumerate(X):
        for vb,b in ((0,r8),(1,q8)):
            bi=enc(b); mm=N[bi]
            while mm:
                j=(mm&-mm).bit_length()-1; mm&=mm-1
                if j not in Xset: continue
                y=dec(j)
                for ha,a in ((0,r8),(1,q8)):
                    if adj5(x,a):
                        add_clause(lit(len(X)+i,vb)^1, lit(j,ha)^1)
    # SCC
    import sys
    sys.setrecursionlimit(10000)
    idx=[-1]*(2*nv); low=[0]*(2*nv); on=[False]*(2*nv); st=[]; comp=[-1]*(2*nv); cnt=[0]; cid=[0]
    def dfs(v):
        idx[v]=low[v]=cnt[0]; cnt[0]+=1; st.append(v); on[v]=True
        for w in g[v]:
            if idx[w]==-1: dfs(w); low[v]=min(low[v],low[w])
            elif on[w]: low[v]=min(low[v],idx[w])
        if low[v]==idx[v]:
            while True:
                w=st.pop(); on[w]=False; comp[w]=cid[0]
                if w==v: break
            cid[0]+=1
    for v in range(2*nv):
        if idx[v]==-1: dfs(v)
    for v in range(nv):
        if comp[2*v]==comp[2*v+1]: return None
    return [comp[2*v]>comp[2*v+1] for v in range(nv)]  # True = q
if __name__=='__main__':
    pairs=[]
    for r in range(16807):
        if r in D8: continue
        # find q candidates: blockers subset D8|{r}
        mm=N[r]
        for q in range(16807):
            if q in D8 or q in Q8: continue
            b=N[q]&sum(1<<x for x in D8|{r})
            pass
    # use precomputed low blockers
    blockers={}
    for s in range(16807):
        if s in {enc(w) for w in R}: continue
        m=N[s]; bl=[]
        while m:
            b=(m&-m).bit_length()-1; m&=m-1
            if b in {enc(w) for w in R}: bl.append(b)
        blockers[s]=frozenset(bl)
    cands=[(r,q) for r in range(16807) if r not in D8 for q,bl in blockers.items() if q not in Q8 and bl and bl<=(D8|{r})]
    print('cands',len(cands),flush=True)
    nsat=0
    for (r8i,q8i) in cands:
        r8=dec(r8i); q8=dec(q8i)
        T=fixed_groups(r8); TB=groups_bits(T)
        sol=solve(r8,q8,TB)
        if sol is not None:
            nsat+=1
            print('SAT',r8,q8,flush=True)
    print('sat count',nsat)
