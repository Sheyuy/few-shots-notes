import sys,time
from ortools.sat.python import cp_model
def solve(L,limit,workers=8,hint=None):
    N=29; m=cp_model.CpModel()
    p=[m.NewIntVar(0,L,f"p{i}") for i in range(N)]
    m.Add(p[0]==0); m.Add(p[N-1]==L)
    for i in range(N-1): m.Add(p[i]+1<=p[i+1])
    diffs=[]
    for i in range(N):
        for j in range(i+1,N):
            d=m.NewIntVar(1,L,f"d{i}_{j}"); m.Add(d==p[j]-p[i]); diffs.append(d)
    m.AddAllDifferent(diffs)
    if hint:
        for i,v in enumerate(hint): m.AddHint(p[i],v)
    s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=limit; s.parameters.num_search_workers=workers
    t=time.time(); st=s.Solve(m); el=time.time()-t
    sol=[s.Value(p[i]) for i in range(N)] if st in (cp_model.OPTIMAL,cp_model.FEASIBLE) else None
    return s.StatusName(st),el,sol
if __name__=="__main__":
    L=int(sys.argv[1]); lim=float(sys.argv[2]); w=int(sys.argv[3]) if len(sys.argv)>3 else 8
    st,el,sol=solve(L,lim,w)
    print(f"L={L} workers={w} status={st} time={el:.1f}s"); print("marks:",sol)
