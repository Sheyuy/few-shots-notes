import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, itertools
from ortools.sat.python import cp_model

P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
def add(a,b): return tuple((x+y)%3 for x,y in zip(a,b))
def neg(a): return tuple((-x)%3 for x in a)
S=[[p[1:] for p in P if p[0]==c] for c in range(3)]
print('slice sizes', [len(s) for s in S])

def allowed(c):
    others=[S[i] for i in range(3) if i!=c]
    forb=set()
    for b in others[0]:
        for d in others[1]:
            forb.add(neg(add(b,d)))
    return [p for p in itertools.product(range(3),repeat=7) if p not in forb], forb

def capcheck(pts):
    st=set(pts)
    for a,b in itertools.combinations(pts,2):
        if neg(add(a,b)) in st: return False
    return True

for c in range(3):
    A,forb=allowed(c)
    print(f'slice {c}: allowed {len(A)}, current {len(S[c])}, current cap {capcheck(S[c])}, current subset {set(S[c])<=set(A)}')
    # CP-SAT max cap on A with size >= len(S[c])+1
    idx={p:i for i,p in enumerate(A)}
    m=cp_model.CpModel(); x=[m.NewBoolVar(f'x{i}') for i in range(len(A))]
    cnt=0
    for i in range(len(A)):
        for j in range(i+1,len(A)):
            k=neg(add(A[i],A[j]))
            if k in idx and idx[k]>j:
                m.AddBoolOr([x[i].Not(),x[j].Not(),x[idx[k]].Not()]); cnt+=1
    m.Add(sum(x)>=len(S[c])+1)
    s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=120; s.parameters.num_search_workers=8
    st=s.Solve(m)
    print(f'  target {len(S[c])+1}: {s.StatusName(st)} (clauses {cnt})')
    if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
        pts=[A[i] for i in range(len(A)) if s.Value(x[i])==1]
        print('  size',len(pts),'valid',capcheck(pts))
        json.dump(pts,open(f'slice{c}_pts.json','w'))
