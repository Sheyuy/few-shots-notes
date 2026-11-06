import sys, itertools
sys.path.insert(0,'/tmp/lbs-repo2/c7')
PAIRS = (
 ((1,3,4,4,6),(2,3,5,4,6)),
 ((3,4,0,3,5),(2,4,6,3,5)),
 ((5,3,1,3,4),(5,3,2,3,5)),
 ((4,4,6,1,6),(5,4,6,0,6)),
 ((6,0,6,4,5),(6,1,6,5,5)),
 ((0,3,5,6,5),(6,3,5,0,5)),
 ((6,4,3,4,0),(6,4,2,4,6)),
 ((6,4,5,3,2),(6,5,5,3,1)),
)
R=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]
R=set(R)
deleted={u for u,v in PAIRS}
B=[w for w in R if w not in deleted]
def tr(w): return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])
X=[tr(w) for w in R]
X[267]=(1,5,6,3,5)
def conf(x,P):
    return any(max(min((a-b)%7,(b-a)%7) for a,b in zip(x,y))<=1 for y in P)
# bitset neighborhoods over suffix space 0..16806
N=[0]*16807
def dec(x):
    d=[0]*5
    for i in range(4,-1,-1): d[i]=x%7; x//=7
    return d
def enc(d):
    r=0
    for v in d: r=r*7+v
    return r
for s in range(16807):
    d=dec(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        e=[(d[i]+off[i])%7 for i in range(5)]
        m |= 1<<enc(e)
    N[s]=m
print("N built", flush=True)
def check(P):
    # P: set of 10-tuples
    T={}
    for x in P:
        p=enc(x[:5]); s=enc(x[5:])
        T.setdefault(p,set()).add(s)
    U={}
    for p,ss in T.items():
        u=0
        for s in ss: u|=N[s]
        U[p]=u
    for p,ss in T.items():
        tb=0
        for s in ss: tb|=1<<s
        for s in ss:
            if (tb & N[s]) != (1<<s): return False,"within p=%d"%p
        m=N[p]
        while m:
            b=(m & -m).bit_length()-1; m&=m-1
            if b==p: continue
            if b in T and (tb & U[b]): return False,"cross p=%d q=%d"%(p,b)
    return True,len(P)
best=(0,None)
for J0 in itertools.combinations(range(8),3):
    J1=[j for j in range(8) if j not in J0]
    PH=[PAIRS[j][0] for j in J0]+[PAIRS[j][1] for j in J1]
    PV=[PAIRS[j][1] for j in J0]+[PAIRS[j][0] for j in J1]
    A=set(x for x in X if conf(x,PV))
    D=set(x for x in X if conf(x,PH))
    C=set()
    for b1 in B:
        for b2 in B: C.add(b1+b2)
    for x in X:
        for j in range(8):
            inJ0=j in J0
            s=((inJ0 and x in A) or ((not inJ0) and x in D))
            s2=((inJ0 and x in D) or ((not inJ0) and x in A))
            C.add(x+PAIRS[j][s]); C.add(PAIRS[j][s2]+x)
    ok,info=check(C)
    print(J0,len(C),ok,info,flush=True)
    if ok and len(C)>best[0]: best=(len(C),J0)
print("BEST",best)
