from ortools.sat.python import cp_model

def third(a,b,d):
    r=0; p=1
    for _ in range(d):
        r += ((-(a%3+b%3))%3)*p; p*=3; a//=3; b//=3
    return r

def solve(d,K,limit=60):
    N=3**d
    m=cp_model.CpModel(); x=[m.NewBoolVar(f'x{i}') for i in range(N)]
    cnt=0
    for a in range(N):
        for b in range(a+1,N):
            c=third(a,b,d)
            if c>b:
                m.AddBoolOr([x[a].Not(),x[b].Not(),x[c].Not()]); cnt+=1
    m.Add(sum(x)>=K)
    s=cp_model.CpSolver(); s.parameters.max_time_in_seconds=limit; s.parameters.num_search_workers=8
    st=s.Solve(m)
    print(f'd={d} N={N} clauses={cnt} K={K} -> {s.StatusName(st)}', flush=True)
    if st in (cp_model.FEASIBLE,cp_model.OPTIMAL):
        print(' found', sum(s.Value(x[i]) for i in range(N)))

print('third(100,200) check placeholder')
solve(3,10,20)   # cap(3)=9 -> asking 10 should be INFEASIBLE
solve(4,21,60)   # cap(4)=20 -> 21 INFEASIBLE
solve(5,46,120)  # cap(5)=45 -> 46 INFEASIBLE
