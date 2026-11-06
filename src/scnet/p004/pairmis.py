import itertools, sys
from ortools.sat.python import cp_model
NS=16807; FULL=(1<<NS)-1
def enc(d): 
    r=0
    for x in d: r=r*7+x
    return r
# build neighbor masks
print("building Nmask...", flush=True)
Nmask=[0]*NS
for s in range(NS):
    d=[0]*5; x=s
    for i in range(4,-1,-1): d[i]=x%7; x//=7
    m=0
    for e in itertools.product(*[[(v-1)%7,v,(v+1)%7] for v in d]):
        m |= 1<<enc(e)
    Nmask[s]=m
print("Nmask done", flush=True)
T=[0]*NS; gcnt=[0]*NS; pts=[]
for line in open('/tmp/p004_points.txt'):
    line=line.strip()
    if not line: continue
    d=[int(c) for c in line]
    p=enc(d[:5]); s=enc(d[5:])
    T[p]|=1<<s; gcnt[p]+=1; pts.append((p,s))
U=[0]*NS
for p,s in pts: U[p]|=Nmask[s]
def nbrs(x):
    return [ (Nmask[x]>>i)&1 and i for i in range(NS) ]  # slow; use bit iteration
def nbrlist(x):
    m=Nmask[x]; out=[]
    while m:
        b=(m & -m).bit_length()-1
        out.append(b); m &= m-1
    return out
groups=[p for p in range(NS) if gcnt[p]>0]
print("groups",len(groups), flush=True)
small=[p for p in groups if gcnt[p]<=26]
print("small",len(small), flush=True)
def allowed(p,q):
    # B excluding p,q
    B=0
    for r in nbrlist(p):
        if r==p or r==q: continue
        B |= U[r]
    return FULL & ~B
def solve_window(p,q,time_limit=10.0):
    Ap=allowed(p,q); Aq=allowed(q,p)
    Vp=[s for s in range(NS) if (Ap>>s)&1]
    Vq=[s for s in range(NS) if (Aq>>s)&1]
    cur=gcnt[p]+gcnt[q]
    if len(Vp)+len(Vq)<=cur:
        return cur, None
    m=cp_model.CpModel()
    xp=[m.NewBoolVar(f'p{i}') for i in range(len(Vp))]
    xq=[m.NewBoolVar(f'q{i}') for i in range(len(Vq))]
    # within p
    for i in range(len(Vp)):
        mi=Nmask[Vp[i]]
        for j in range(i+1,len(Vp)):
            if (mi>>Vp[j])&1: m.Add(xp[i]+xp[j]<=1)
    for i in range(len(Vq)):
        mi=Nmask[Vq[i]]
        for j in range(i+1,len(Vq)):
            if (mi>>Vq[j])&1: m.Add(xq[i]+xq[j]<=1)
    # cross: every p-suffix vs q-suffix are adjacent because p,q adjacent prefixes
    for i in range(len(Vp)):
        mi=Nmask[Vp[i]]
        for j in range(len(Vq)):
            if (mi>>Vq[j])&1: m.Add(xp[i]+xq[j]<=1)
    m.Maximize(sum(xp)+sum(xq))
    sol=cp_model.CpSolver(); sol.parameters.max_time_in_seconds=time_limit
    st=sol.Solve(m)
    if st in (cp_model.OPTIMAL,cp_model.FEASIBLE):
        val=int(round(sol.ObjectiveValue()))
        chosen=[]
        for i,v in enumerate(xp):
            if sol.Value(v): chosen.append((p,Vp[i]))
        for i,v in enumerate(xq):
            if sol.Value(v): chosen.append((q,Vq[i]))
        return val, chosen
    return None,None
best=0
res={}
count=0
for i,p in enumerate(small):
    for q in nbrlist(p):
        if q<=p or gcnt[q]==0 or gcnt[q]>26: continue
        count+=1
        val,ch=solve_window(p,q,time_limit=5.0)
        cur=gcnt[p]+gcnt[q]
        print(f"pair {p}({gcnt[p]}) {q}({gcnt[q]}) cur={cur} mis={val}", flush=True)
        if val is not None and val>cur:
            print("IMPROVEMENT!",p,q,cur,val, flush=True)
        if val is not None and val>best: best=val
print("pairs checked",count,"best",best, flush=True)
