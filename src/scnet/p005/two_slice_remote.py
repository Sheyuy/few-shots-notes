import json, time, sys
from ortools.sat.python import cp_model
f=int(sys.argv[1]); workers=int(sys.argv[2]); tlim=int(sys.argv[3]); tag=sys.argv[4]
idx=[int(l) for l in open('baseline.idx')]
P=[]
for i in idx:
    q=[0]*8
    for k in range(7,-1,-1): q[k]=i%3; i//=3
    P.append(tuple(q))
S=[[p[1:] for p in P if p[0]==c] for c in range(3)]
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
def pt(i):
    p=[0]*7
    for k in range(6,-1,-1): p[k]=i%3; i//=3
    return p
o=[i for i in range(3) if i!=f]
target=513-len(S[f])
t0=time.time()
m=cp_model.CpModel(); y=[m.NewBoolVar(f'y{i}') for i in range(N)]; z=[m.NewBoolVar(f'z{i}') for i in range(N)]
for a in range(N):
    for b in range(a+1,N):
        c=third(a,b)
        if c>b:
            m.AddBoolOr([y[a].Not(),y[b].Not(),y[c].Not()]); m.AddBoolOr([z[a].Not(),z[b].Not(),z[c].Not()])
for a in [negidx(enc(p)) for p in S[f]]:
    for p in range(N):
        q=0; pp=p; aa=a; mul=1
        for k in range(d):
            qv=(aa%3-pp%3)%3; q+=qv*mul; mul*=3; pp//=3; aa//=3
        m.AddBoolOr([y[p].Not(),z[q].Not()])
m.Add(sum(y)+sum(z)>=target)
print('f',f,'target',target,'built %.1fs'%(time.time()-t0),flush=True)
s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=tlim; s.parameters.num_search_workers=workers
st=s.Solve(m); print('status',s.StatusName(st),'t=%.1f'%(time.time()-t0),flush=True)
if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
    B1=[i for i in range(N) if s.Value(y[i])==1]; B2=[i for i in range(N) if s.Value(z[i])==1]
    pts=[[f]+list(p) for p in S[f]]+[[o[0]]+pt(i) for i in B1]+[[o[1]]+pt(i) for i in B2]
    print('total',len(pts),flush=True)
    json.dump({'points':pts},open(f'cp_cert_{tag}.json','w'))
