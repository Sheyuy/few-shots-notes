#!/usr/bin/env python3
"""Extract a 36-vertex (4,6)-Ramsey graph from a minisat solution and verify it.

The CNF was built by sym_sat.py: variable x_o = 1 means the edge orbit o is
colour 1 ("friend").  A SAT solution therefore gives, for every edge of K_36,
its colour; colour 1 = edge, colour 2 = non-edge.  The resulting graph must
contain no K4 and no independent set of 6.

usage: extract.py <cnf-basename>      # reads <name>.cnf, <name>.json, <name>.sat
"""
import itertools, json, os, sys

N = 36


def read_cnf(path):
    nv = nc = 0
    with open(path) as f:
        for line in f:
            if line.startswith("p cnf"):
                _, _, nv, nc = line.split()
                nv, nc = int(nv), int(nc)
            elif line.startswith("c") or not line.strip():
                continue
    return nv, nc


def read_sat(path):
    """Parse a SAT model.  Two formats occur:
         CryptoMiniSat:  'v 1 -2 3 ... 0'
         minisat:        'SAT\n1 -2 3 ... 0'
    """
    vals = {}
    with open(path) as f:
        for line in f:
            line = line.strip()
            if line.startswith("v "):
                toks = line[2:].split()
            elif line.startswith("SAT") or line.startswith("s SATISFIABLE"):
                toks = []
            elif line and line[0] in "0123456789-":
                toks = line.split()
            else:
                continue
            for tok in toks:
                try:
                    vals[int(tok)] = True
                except ValueError:
                    pass
    return vals


def build_graph(satfile, jsonfile):
    meta = json.load(open(jsonfile))
    orb = meta["orb"]
    nv = meta["nvar"]
    # number of vertices: the orbit list has one entry per edge of K_n
    n_edges = len(orb)
    n = int((1 + (1 + 8 * n_edges) ** 0.5) / 2)
    if n * (n - 1) // 2 != n_edges:
        raise ValueError("orbit data is inconsistent")
    vals = read_sat(satfile)
    # a variable is true if any of its literals appears in the solution
    var_true = {}
    for lit in vals:
        if lit == 0:            # DIMACS terminator
            continue
        v = abs(lit) - 1        # DIMACS literals are 1-based; variable o -> literal o+1
        sign = 1 if lit > 0 else -1
        var_true.setdefault(v, set()).add(sign)
    colour = {}
    for o in range(nv + 1):
        s = var_true.get(o)
        if s is None:
            colour[o] = 1          # unassigned -> default colour 1
        else:
            colour[o] = 1 if (1 in s) else 2
    edges = list(itertools.combinations(range(n), 2))
    adj = [set() for _ in range(n)]
    for i, (a, b) in enumerate(edges):
        if colour[orb[i]] == 1:
            adj[a].add(b)
            adj[b].add(a)
    return adj


def check(adj):
    n = len(adj)
    k4 = [c for c in itertools.combinations(range(n), 4)
          if all(b in adj[a] for a, b in itertools.combinations(c, 2))]
    i6 = [c for c in itertools.combinations(range(n), 6)
          if all(b not in adj[a] for a, b in itertools.combinations(c, 2))]
    return k4, i6


def main():
    base = sys.argv[1]
    cnf, js, sat = base + ".cnf", base + ".json", base + ".sat"
    if not os.path.exists(sat):
        print("no .sat file yet:", sat); return 1
    adj = build_graph(sat, js)
    k4, i6 = check(adj)
    edges = sum(len(a) for a in adj) // 2
    print(f"vertices={len(adj)} edges={edges} 4-cliques={len(k4)} independent-6={len(i6)}")
    if k4:
        print("  sample 4-cliques:", k4[:3])
    if i6:
        print("  sample independent 6-sets:", i6[:3])
    need = int(os.environ.get("MIN_VERTICES", "36"))
    ok = (len(adj) >= need and not k4 and not i6)
    print("VALID CERTIFICATE" if ok else "not valid")
    if ok:
        out = base + "_graph.json"
        json.dump({"adjacency": ["".join("1" if j in adj[i] else "0" for j in range(len(adj)))
                                for i in range(len(adj))]}, open(out, "w"))
        print("wrote", out)
    return 0 if ok else 2


if __name__ == "__main__":
    sys.exit(main())
