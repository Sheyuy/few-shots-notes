import sys, time
from ortools.sat.python import cp_model

def solve(L, limit, hint=None, workers=8, log=False):
    N=29
    m=cp_model.CpModel()
    p=[m.NewIntVar(0,L,f"p{i}") for i in range(N)]
    m.Add(p[0]==0); m.Add(p[N-1]==L)
    for i in range(N-1): m.Add(p[i]+1<=p[i+1])
    diffs=[]
    for i in range(N):
        for j in range(i+1,N):
            d=m.NewIntVar(1,L,f"d{i}_{j}")
            m.Add(d==p[j]-p[i]); diffs.append(d)
    m.AddAllDifferent(diffs)
    if hint:
        for i,v in enumerate(hint): m.AddHint(p[i],v)
    s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=limit
    s.parameters.num_search_workers=workers
    s.parameters.log_search_progress=log
    t=time.time(); st=s.Solve(m); el=time.time()-t
    return s.StatusName(st), el, ([s.Value(p[i]) for i in range(N)] if st in (cp_model.OPTIMAL,cp_model.FEASIBLE) else None)

if __name__=="__main__":
    L=int(sys.argv[1]); lim=float(sys.argv[2]) if len(sys.argv)>2 else 60
    hint=[0,3,28,33,41,95,156,158,193,207,233,252,267,338,339,355,394,403,421,473,479,502,523,570,580,592,612,616,623]
    if L!=623: hint=None
    st,el,sol=solve(L,lim,hint)
    print(f"L={L} status={st} time={el:.1f}s")
    print("marks:",sol)
