import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import time, sys, json
from ortools.sat.python import cp_model

d=8; N=3**d; K=513
t0=time.time()
def third(a,b):
    r=0
    for _ in range(d):
        r=r*3+((-(a%3+b%3))%3); a//=3; b//=3
    return r

model=cp_model.CpModel()
x=[model.NewBoolVar(f'x{i}') for i in range(N)]
cnt=0
for a in range(N):
    for b in range(a+1,N):
        c=third(a,b)
        if c>b:
            model.AddBoolOr([x[a].Not(),x[b].Not(),x[c].Not()])
            cnt+=1
print(f'built {cnt} clauses in {time.time()-t0:.1f}s', flush=True)

# WLOG 0 in S and e1 in S
model.Add(x[0]==1)
model.Add(x[1]==1)
model.Add(sum(x) >= K)

solver=cp_model.CpSolver()
solver.parameters.max_time_in_seconds = 900
solver.parameters.num_search_workers = 8
solver.parameters.log_search_progress = True
print('solving...', flush=True)
status=solver.Solve(model)
print('status', solver.StatusName(status), 't=%.1f'%(time.time()-t0), flush=True)
if status in (cp_model.OPTIMAL, cp_model.FEASIBLE):
    pts=[i for i in range(N) if solver.Value(x[i])==1]
    print('size',len(pts))
    json.dump(pts,open('cp_points.json','w'))
    def point(i):
        p=[0]*8
        for k in range(7,-1,-1): p[k]=i%3; i//=3
        return p
    json.dump({'points':[point(i) for i in pts]},open('cp_cert.json','w'))
    print('saved')
