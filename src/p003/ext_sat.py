import itertools, json, sys, time
from pysat.solvers import Cadical195

def load(path):
    f=open(path); n=int(f.readline()); rows=[f.readline().strip() for _ in range(n)]
    return n,[[c=='1' for c in r] for r in rows]

def solve(seed, tag):
    n,A=load(seed)
    clauses=[]
    for tri in itertools.combinations(range(n),3):
        a,b,c=tri
        if A[a][b] and A[a][c] and A[b][c]:
            clauses.append([-(a+1),-(b+1),-(c+1)])
    nk=len(clauses)
    for q in itertools.combinations(range(n),5):
        ok=True
        for a,b in itertools.combinations(q,2):
            if A[a][b]: ok=False; break
        if ok:
            clauses.append([p+1 for p in q])
    s=Cadical195(bootstrap_with=clauses)
    res=s.solve()
    print(tag,"clauses",len(clauses),"k4c",nk,"->",res,flush=True)
    if res:
        model=set(l for l in s.get_model() if l>0)
        S=[i for i in range(n) if (i+1) in model]
        rows=[]
        for i in range(n):
            rows.append(''.join('1' if A[i][j] else '0' for j in range(n)) + ('1' if i in S else '0'))
        rows.append(''.join('1' if j in S else '0' for j in range(n)) + '0')
        json.dump({"adjacency":rows}, open("ext_%s_certificate.json"%tag,"w"))
        print(tag,"SAT S=",S,flush=True)
        return True
    return False

t0=time.time()
for i in range(37):
    if solve("seeds/g%d.txt"%i, "g%d"%i):
        print("FOUND via seed",i)
        sys.exit(0)
print("all 37 unsat, %.1fs"%(time.time()-t0))