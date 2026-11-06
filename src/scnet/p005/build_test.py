import time
from ortools.sat.python import cp_model

def third(a,b,d):
    r=0
    for k in range(d):
        r=r*3+((-(a%3+b%3))%3); a//=3; b//=3
    return r

for d in (4,5):
    N=3**d
    t0=time.time()
    model=cp_model.CpModel()
    x=[model.NewBoolVar(f'x{i}') for i in range(N)]
    cnt=0
    for a in range(N):
        for b in range(a+1,N):
            c=third(a,b,d)
            if c>b:
                model.AddBoolOr([x[a].Not(),x[b].Not(),x[c].Not()])
                cnt+=1
    t1=time.time()
    print(f'd={d} N={N} clauses={cnt} build={t1-t0:.2f}s')
