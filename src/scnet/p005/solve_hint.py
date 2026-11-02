import time, json
from ortools.sat.python import cp_model
d=8; N=6561; K=513
def third(a,b):
    r=0;p=1
    for _ in range(d):
        r+=((-(a%3+b%3))%3)*p;p*=3;a//=3;b//=3
    return r
base=[int(x) for x in open('baseline.idx')]
bset=set(base)
t0=time.time()
m=cp_model.CpModel(); x=[m.NewBoolVar(f'x{i}') for i in range(N)]
cnt=0
for a in range(N):
    for b in range(a+1,N):
        c=third(a,b)
        if c>b:
            m.AddBoolOr([x[a].Not(),x[b].Not(),x[c].Not()]);cnt+=1
m.Add(sum(x)>=K)
for i in range(N):
    m.AddHint(x[i], 1 if i in bset else 0)
print('built',cnt,'%.1fs'%(time.time()-t0),flush=True)
s=cp_model.CpSolver()
s.parameters.max_time_in_seconds=300
s.parameters.num_search_workers=8
s.parameters.log_search_progress=False
st=s.Solve(m)
print('status',s.StatusName(st),'t=%.1f'%(time.time()-t0),flush=True)
if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
    pts=[i for i in range(N) if s.Value(x[i])==1]
    print('size',len(pts),flush=True)
    def point(i):
        p=[0]*8
        for k in range(7,-1,-1):p[k]=i%3;i//=3
        return p
    json.dump({'points':[point(i) for i in pts]},open('cp_hint_cert.json','w'))
