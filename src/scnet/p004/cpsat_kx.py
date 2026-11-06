"""CP-SAT model: does R367 admit a (k -> k+1) exchange?
Choose A (external points) with |A| = k+1, pairwise non-adjacent, and U subset of S
with |U| <= k such that T(v) subset U for every v in A.  Equivalent to the DFS in xk.c.
Usage: cpsat_kx.py K [seconds] [seed]
"""
import sys, time
from ortools.sat.python import cp_model

N5 = 16807
def digs(v):
    d = []
    for _ in range(5):
        d.append(v % 7); v //= 7
    return d

def build(path):
    S = []
    for line in open(path):
        p = line.split()
        if len(p) != 5: continue
        x = 0
        for i in range(4, -1, -1):
            x = x * 7 + int(p[i])
        S.append(x)
    return S

def main():
    k = int(sys.argv[1]); limit = float(sys.argv[2]) if len(sys.argv) > 2 else 300.0
    seed = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    path = "/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004/R367.txt"
    S = build(path)
    assert len(S) == 367, len(S)
    inS = bytearray(N5); 
    for s in S: inS[s] = 1
    D = [digs(v) for v in range(N5)]
    def adj(a, b):
        any_ = False
        da, db = D[a], D[b]
        for i in range(5):
            d = da[i] - db[i]
            if d < 0: d = -d
            if d == 6: d = 1
            if d > 1: return False
            if d == 1: any_ = True
        return any_
    L = []; T = []
    for v in range(N5):
        if inS[v]: continue
        t = [s for s in S if adj(v, s)]
        if len(t) <= k:
            L.append(v); T.append(t)
    m = len(L)
    print(f"k={k} candidates={m} maxT={max(len(t) for t in T)}", flush=True)

    model = cp_model.CpModel()
    a = [model.NewBoolVar(f"a{i}") for i in range(m)]
    Sset = sorted({s for t in T for s in t})
    pos = {s: i for i, s in enumerate(Sset)}
    u = [model.NewBoolVar(f"u{i}") for i in range(len(Sset))]
    model.Add(sum(a) == k + 1)
    model.Add(sum(u) <= k)
    for i, t in enumerate(T):
        for s in t:
            model.AddImplication(a[i], u[pos[s]])
    # WLOG every used u is justified by some chosen point
    back = {s: [] for s in Sset}
    for i, t in enumerate(T):
        for s in t: back[s].append(i)
    for s in Sset:
        model.AddBoolOr([u[pos[s]]] + [a[i].Not() for i in back[s]]).OnlyEnforceIf(u[pos[s]])
    idx = {v: i for i, v in enumerate(L)}
    conflicts = 0
    for v in L:
        for w in S:
            pass
    for i, v in enumerate(L):
        for j in range(i + 1, m):
            w = L[j]
            if adj(v, w):
                model.AddBoolOr([a[i].Not(), a[j].Not()]); conflicts += 1
    print(f"vars a={m} u={len(Sset)} conflicts={conflicts}", flush=True)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = limit
    solver.parameters.num_search_workers = 8
    solver.parameters.random_seed = seed
    t0 = time.time()
    st = solver.Solve(model)
    print(f"status={solver.StatusName(st)} wall={time.time()-t0:.1f}s "
          f"conflicts={solver.NumConflicts()} branches={solver.NumBranches()}", flush=True)
    if st in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        A = [L[i] for i in range(m) if solver.Value(a[i])]
        U = [Sset[i] for i in range(len(Sset)) if solver.Value(u[i])]
        print("A=", [D[v] for v in A])
        print("U=", [D[s] for s in U])

main()
