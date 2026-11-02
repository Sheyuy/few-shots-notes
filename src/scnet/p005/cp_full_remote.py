import time, json, sys
from ortools.sat.python import cp_model
d=8; N=3**d
K=int(sys.argv[1]); workers=int(sys.argv[2]); tlim=int(sys.argv[3]); tag=sys.argv[4]; seed=int(sys.argv[5])
def third(a,b):
    r=0; p=1
    for _ in range(d):
        r += ((-(a%3+b%3))%3)*p; p*=3; a//=3; b//=3
    return r
t0=time.time()
m=cp_model.CpModel(); x=[m.NewBoolVar(f'x{i}') for i in range(N)]
for a in range(N):
    for b in range(a+1,N):
        c=third(a,b)
        if c>b: m.AddBoolOr([x[a].Not(),x[b].Not(),x[c].Not()])
print('built %.1fs'%(time.time()-t0),flush=True)
m.Add(x[0]==1); m.Add(x[1]==1); m.Add(sum(x)>=K)
s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=tlim; s.parameters.num_search_workers=workers; s.parameters.random_seed=seed
st=s.Solve(m)
print('status',s.StatusName(st),'t=%.1f'%(time.time()-t0),flush=True)
if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
    pts=[i for i in range(N) if s.Value(x[i])==1]
    def point(i):
        p=[0]*8
        for k in range(7,-1,-1): p[k]=i%3; i//=3
        return p
    json.dump({'points':[point(i) for i in pts]},open(f'cp_cert_{tag}.json','w'))
    print('saved',len(pts),flush=True)
