import itertools, json, random, sys, time
from pysat.solvers import Kissat404, Cadical195

def perm_of(kind, n=36, seed=1):
    random.seed(seed)
    v = list(range(n)); random.shuffle(v)
    p = [0]*n
    if kind == "inv0":            # 18 transpositions
        for k in range(0, n, 2): p[v[k]] = v[k+1]; p[v[k+1]] = v[k]
    elif kind == "inv4":          # 4 fixed points + 16 transpositions
        for k in range(4): p[v[k]] = v[k]
        for k in range(4, n, 2): p[v[k]] = v[k+1]; p[v[k+1]] = v[k]
    elif kind == "ord2fix2":      # 2 fixed points + 17 transpositions
        for k in range(2): p[v[k]] = v[k]
        for k in range(2, n, 2): p[v[k]] = v[k+1]; p[v[k+1]] = v[k]
    elif kind == "ord3fix":       # 3 fixed points + 11 3-cycles
        for k in range(3): p[v[k]] = v[k]
        for k in range(3, n, 3):
            p[v[k]] = v[k+1]; p[v[k+1]] = v[k+2]; p[v[k+2]] = v[k]
    elif kind == "ord4fix":       # 4 fixed points + 8 4-cycles
        for k in range(4): p[v[k]] = v[k]
        for k in range(4, n, 4):
            p[v[k]] = v[k+1]; p[v[k+1]] = v[k+2]; p[v[k+2]] = v[k+3]; p[v[k+3]] = v[k]
    elif kind == "ord4_22":       # two 2-cycles + eight 4-cycles
        p[v[0]] = v[1]; p[v[1]] = v[0]
        p[v[2]] = v[3]; p[v[3]] = v[2]
        for k in range(4, n, 4):
            p[v[k]] = v[k+1]; p[v[k+1]] = v[k+2]; p[v[k+2]] = v[k+3]; p[v[k+3]] = v[k]
    elif kind == "ord6_32":       # two 3-cycles + five 6-cycles
        p[v[0]] = v[1]; p[v[1]] = v[2]; p[v[2]] = v[0]
        p[v[3]] = v[4]; p[v[4]] = v[5]; p[v[5]] = v[3]
        for k in range(6, n, 6):
            for t in range(5): p[v[k+t]] = v[k+t+1]
            p[v[k+5]] = v[k]
    elif kind == "ord6_2":        # one 6-cycle + fifteen 2-cycles
        for t in range(5): p[v[t]] = v[t+1]
        p[v[5]] = v[0]
        for k in range(6, n, 2): p[v[k]] = v[k+1]; p[v[k+1]] = v[k]
    elif kind == "ord3":          # 12 3-cycles
        for k in range(0, n, 3):
            p[v[k]] = v[k+1]; p[v[k+1]] = v[k+2]; p[v[k+2]] = v[k]
    elif kind == "ord4":          # 9 4-cycles
        for k in range(0, n, 4):
            p[v[k]] = v[k+1]; p[v[k+1]] = v[k+2]; p[v[k+2]] = v[k+3]; p[v[k+3]] = v[k]
    elif kind == "ord6":          # 6 6-cycles
        for k in range(0, n, 6):
            for t in range(5): p[v[k+t]] = v[k+t+1]
            p[v[k+5]] = v[k]
    else:
        raise SystemExit("kind")
    return p

def orbits(n, p):
    mp = {}
    for i in range(n):
        for j in range(i+1, n):
            if (i, j) in mp: continue
            orb = set(); a, b = i, j
            while True:
                orb.add((min(a, b), max(a, b)))
                a, b = p[a], p[b]
                if (min(a, b), max(a, b)) == (i, j): break
            rep = min(orb)
            for e in orb: mp[e] = rep
    reps = sorted(set(mp.values()))
    var = {r: k+1 for k, r in enumerate(reps)}
    return mp, var

def build(kind, seed=1):
    n = 36
    p = perm_of(kind, n, seed)
    mp, var = orbits(n, p)
    clauses = set()
    for comb in itertools.combinations(range(n), 4):
        lits = tuple(sorted(set(-var[mp[(a, b)]] for a, b in itertools.combinations(comb, 2))))
        clauses.add(lits)
    nk = len(clauses)
    for comb in itertools.combinations(range(n), 6):
        lits = tuple(sorted(set(var[mp[(a, b)]] for a, b in itertools.combinations(comb, 2))))
        clauses.add(lits)
    print(kind, "orbits", len(var), "distinct clauses", len(clauses), "(k4 %d)" % nk, flush=True)
    return n, p, mp, var, [list(c) for c in clauses]

def run(kind, seed=1, solver_name="kissat"):
    n, p, mp, var, clauses = build(kind, seed)
    cls = {"kissat": Kissat404, "cadical": Cadical195}[solver_name]
    s = cls(bootstrap_with=clauses)
    t0 = time.time(); res = s.solve()
    print(kind, "seed", seed, "solve", res, "%.1fs" % (time.time()-t0), flush=True)
    if res:
        model = set(l for l in s.get_model() if l > 0)
        rows = []
        for i in range(n):
            rows.append(''.join('1' if i != j and var[mp[(min(i,j),max(i,j))]] in model else '0' for j in range(n)))
        fn = "symq_%s_%d_certificate.json" % (kind, seed)
        json.dump({"adjacency": rows}, open(fn, "w"))
        print("wrote", fn, flush=True)
        return True
    return False

if __name__ == "__main__":
    run(sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 1,
        sys.argv[3] if len(sys.argv) > 3 else "kissat")