import json, time, itertools, random
from ortools.sat.python import cp_model

def third(a,b,d):
    r=0;p=1
    for _ in range(d):
        r+=((-(a%3+b%3))%3)*p;p*=3;a//=3;b//=3
    return r

def solve_cap(d,K,limit=120):
    N=3**d
    m=cp_model.CpModel(); x=[m.NewBoolVar(f'x{i}') for i in range(N)]
    for a in range(N):
        for b in range(a+1,N):
            c=third(a,b,d)
            if c>b: m.AddBoolOr([x[a].Not(),x[b].Not(),x[c].Not()])
    m.Add(sum(x)>=K)
    s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=limit; s.parameters.num_search_workers=8
    st=s.Solve(m)
    print('d',d,'K',K,s.StatusName(st),flush=True)
    if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
        return [i for i in range(N) if s.Value(x[i])==1]
    return None

t0=time.time()
A=solve_cap(6,112,120)
print('A size',len(A) if A else None,'t=%.0f'%(time.time()-t0))
if A:
    json.dump(A,open('cap6_112.json','w'))
    def dig(i,d):
        p=[0]*d
        for k in range(d-1,-1,-1): p[k]=i%3; i//=3
        return p
    S=set()
    for a in A:
        da=dig(a,6)
        for b0 in (0,1):
            for b1 in (0,1):
                idx=0
                for v in da+[b0,b1]: idx=idx*3+v
                S.add(idx)
    print('product size',len(S))
    # build conf on F_3^8
    N8=6561
    conf=[0]*N8
    Sl=list(S)
    for i,a in enumerate(Sl):
        for b in Sl[i+1:]:
            conf[third(a,b,8)]+=1
    # greedy augmentation
    best=0
    for trial in range(200):
        T=set(S); c=conf[:]
        order=list(range(N8)); random.shuffle(order)
        # incremental add
        for p in order:
            if p in T or c[p]!=0: continue
            # add p: update conf
            for q in T:
                c[third(p,q,8)]+=1
            T.add(p)
        if len(T)>best: best=len(T); print('trial',trial,'size',best,flush=True)
        if best>=513:
            print('FOUND',best); json.dump(sorted(T),open('product_best.idx','w')); break
    print('best',best)
