import itertools, collections, sys
PAIRS8=[((1,3,4,4,6),(2,3,5,4,6)),((3,4,0,3,5),(2,4,6,3,5)),((5,3,1,3,4),(5,3,2,3,5)),((4,4,6,1,6),(5,4,6,0,6)),((6,0,6,4,5),(6,1,6,5,5)),((0,3,5,6,5),(6,3,5,0,5)),((6,4,3,4,0),(6,4,2,4,6)),((6,4,5,3,2),(6,5,5,3,1))]
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
def adj(a,b): return a!=b and all(min((x-y)%7,(y-x)%7)<=1 for x,y in zip(a,b))
def conf(x,P): return any(all(min((u-v)%7,(v-u)%7)<=1 for u,v in zip(x,y)) for y in P)
Ridx={enc(w):w for w in R}
N=[0]*16807
for s in range(16807):
    d=dec(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        m|=1<<enc([(d[i]+off[i])%7 for i in range(5)])
    N[s]=m
blockers={}
for s in range(16807):
    if s in Ridx: continue
    m=N[s]; bl=[]
    while m:
        b=(m&-m).bit_length()-1; m&=m-1
        if b in Ridx: bl.append(b)
    blockers[s]=frozenset(bl)
def tr(w): return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])
X0=[tr(w) for w in Rlist]; X0[267]=(1,5,6,3,5)
X=set(X0)
def build(pairs,J0,check_only=False):
    d=len(pairs)
    deleted={enc(r) for r,q in pairs}
    B=[w for w in Rlist if enc(w) not in deleted]
    J1=[j for j in range(d) if j not in J0]
    PH=[pairs[j][0] for j in J0]+[pairs[j][1] for j in J1]
    PV=[pairs[j][1] for j in J0]+[pairs[j][0] for j in J1]
    A=set(x for x in X if conf(x,PV)); Ds=set(x for x in X if conf(x,PH))
    C=set()
    for b1 in B:
        for b2 in B: C.add(b1+b2)
    for x in X0:
        for j in range(d):
            inJ0=j in J0
            s=((inJ0 and x in A) or ((not inJ0) and x in Ds))
            s2=((inJ0 and x in Ds) or ((not inJ0) and x in A))
            C.add(x+pairs[j][s]); C.add(pairs[j][s2]+x)
    return C,len(B),len(A),len(Ds)
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
# baseline sanity
C,nb,na,nd=build(PAIRS8,{0,5,6})
print('baseline',len(C),nb,na,nd,check(C),flush=True)
D8={enc(r) for r,q in PAIRS8}; Q8={enc(q) for r,q in PAIRS8}
cands=[]
for r in Ridx:
    if r in D8: continue
    for s,bl in blockers.items():
        if s in Q8: continue
        if bl and bl<=(D8|{r}): cands.append((r,s))
print('cands',len(cands),flush=True)
best=0
for (r8,s8) in cands:
    r=dec(r8); q=dec(s8)
    for orient in ('J1','J0'):
        J0={0,5,6} if orient=='J1' else {0,5,6,8}
        pairs=PAIRS8+[(r,q)]
        C,nb,na,nd=build(pairs,J0)
        ok=check(C)
        if ok and len(C)>best:
            best=len(C)
            print('VALID',r,q,orient,len(C),nb,na,nd,flush=True)
print('done best',best)
