import json, time, sys, random
from ortools.sat.python import cp_model
seed=int(sys.argv[1]); workers=int(sys.argv[2]); tlim=int(sys.argv[3]); tag=sys.argv[4]
random.seed(seed)
def det3(M):
    n=len(M); A=[row[:] for row in M]; d=1
    for i in range(n):
        p=None
        for r in range(i,n):
            if A[r][i]%3: p=r; break
        if p is None: return 0
        if p!=i: A[i],A[p]=A[p],A[i]; d=-d
        inv=pow(A[i][i],-1,3)
        for r in range(i+1,n):
            f=A[r][i]*inv%3
            for c in range(i,n): A[r][c]=(A[r][c]-f*A[i][c])%3
    return d%3
def inv3(M):
    n=len(M); A=[M[i][:]+[1 if i==j else 0 for j in range(n)] for i in range(n)]
    for i in range(n):
        p=next(r for r in range(i,n) if A[r][i]%3)
        A[i],A[p]=A[p],A[i]
        iv=pow(A[i][i],-1,3); A[i]=[v*iv%3 for v in A[i]]
        for r in range(n):
            if r!=i and A[r][i]%3:
                f=A[r][i]; A[r]=[(A[r][c]-f*A[i][c])%3 for c in range(2*n)]
    return [row[n:] for row in A]
def mul(M,x): return [sum(M[i][j]*x[j] for j in range(8))%3 for i in range(8)]
while True:
    M=[[random.randrange(3) for _ in range(8)] for _ in range(8)]
    if det3(M): break
Minv=inv3(M)
idx=[int(l) for l in open('baseline.idx')]
P0=[]
for i in idx:
    q=[0]*8
    for k in range(7,-1,-1): q[k]=i%3; i//=3
    P0.append(q)
P=[mul(M,x) for x in P0]
S=[[p[1:] for p in P if p[0]==c] for c in range(3)]
sizes=[len(s) for s in S]
f=max(range(3), key=lambda c: sizes[c])
o=[i for i in range(3) if i!=f]
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
target=513-sizes[f]
t0=time.time()
m=cp_model.CpModel(); y=[m.NewBoolVar(f'y{i}') for i in range(N)]; z=[m.NewBoolVar(f'z{i}') for i in range(N)]
for a in range(N):
    for b in range(a+1,N):
        c=third(a,b)
        if c>b:
            m.AddBoolOr([y[a].Not(),y[b].Not(),y[c].Not()]); m.AddBoolOr([z[a].Not(),z[b].Not(),z[c].Not()])
for a in [negidx(enc(p)) for p in S[f]]:
    for p in range(N):
        q=0; pp=p; aa=a; mulp=1
        for k in range(d):
            qv=(aa%3-pp%3)%3; q+=qv*mulp; mulp*=3; pp//=3; aa//=3
        m.AddBoolOr([y[p].Not(),z[q].Not()])
m.Add(sum(y)+sum(z)>=target)
print('seed',seed,'sizes',sizes,'f',f,'target',target,'built %.1fs'%(time.time()-t0),flush=True)
s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=tlim; s.parameters.num_search_workers=workers
st=s.Solve(m); print('status',s.StatusName(st),'t=%.1f'%(time.time()-t0),flush=True)
if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
    B1=[i for i in range(N) if s.Value(y[i])==1]; B2=[i for i in range(N) if s.Value(z[i])==1]
    ys=[[f]+list(p) for p in S[f]]+[[o[0]]+pt(i) for i in B1]+[[o[1]]+pt(i) for i in B2]
    xs=[mul(Minv,y_) for y_ in ys]
    print('total',len(xs),flush=True)
    json.dump({'points':xs},open(f'cp_cert_{tag}.json','w'))
