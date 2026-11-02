import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, time
from ortools.sat.python import cp_model
P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
S=[ [p[1:] for p in P if p[0]==c] for c in range(3)]
d=7; N=3**d
def dig(i):
    p=[0]*d
    for k in range(d-1,-1,-1): p[k]=i%3; i//=3
    return p
D=[dig(i) for i in range(N)]
def enc(p):
    r=0
    for v in p: r=r*3+v
    return r
def third(a,b):
    r=0
    for k in range(d): r=r*3+((-(D[a][k]+D[b][k]))%3)
    return r
def negidx(a):
    r=0
    for k in range(d): r=r*3+((3-D[a][k])%3)
    return r

t0=time.time()
m=cp_model.CpModel()
y=[m.NewBoolVar(f'y{i}') for i in range(N)]
z=[m.NewBoolVar(f'z{i}') for i in range(N)]
# cap constraints in each slice
cnt=0
for a in range(N):
    for b in range(a+1,N):
        c=third(a,b)
        if c>b:
            m.AddBoolOr([y[a].Not(),y[b].Not(),y[c].Not()])
            m.AddBoolOr([z[a].Not(),z[b].Not(),z[c].Not()])
            cnt+=2
# cross: for a in S0, p+q = -a  -> not both
S0set=set(enc(p) for p in S[0])
negS0=[negidx(a) for a in S0set]
cross=0
for a in negS0:
    # pairs p,q with p+q=a: iterate p, q=third? need p+q=a => q = a-p digitwise
    for p in range(N):
        q=0; pp=p; aa=a; mul=1
        ok=True
        for k in range(d):
            qv=(aa%3 - pp%3)%3
            q += qv*mul; mul*=3; pp//=3; aa//=3
        m.AddBoolOr([y[p].Not(),z[q].Not()])
        cross+=1
print('cap clauses',cnt,'cross',cross,'build %.1fs'%(time.time()-t0),flush=True)
m.Add(sum(y)+sum(z)>=321)
s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=7200; s.parameters.num_search_workers=8
print('solve',flush=True)
st=s.Solve(m)
print('status',s.StatusName(st),'t=%.1f'%(time.time()-t0),flush=True)
if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
    B1=[i for i in range(N) if s.Value(y[i])==1]
    B2=[i for i in range(N) if s.Value(z[i])==1]
    print('sizes',len(B1),len(B2),flush=True)
    def pt(i):
        p=[0]*7
        for k in range(6,-1,-1): p[k]=i%3; i//=3
        return p
    pts=[[0]+list(p) for p in S[0]] + [[1]+list(pt(i)) for i in B1] + [[2]+list(pt(i)) for i in B2]
    json.dump({'points':pts},open('two_slice_cert.json','w'))
    print('total',len(pts),flush=True)
