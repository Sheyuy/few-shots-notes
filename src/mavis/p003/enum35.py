#!/usr/bin/env python3
"""
enum35.py -- enumerate all 35-vertex (4,6)-graphs and test each for extension
to 36 vertices.

Why this is the right decomposition
-----------------------------------
Proving "no 36-vertex (4,6)-graph exists" directly is one enormous UNSAT
instance (630 edge variables, 2,006,697 clauses) that our solvers could not
decide in 5.5 h / 10^9 conflicts.  But the problem factors:

    violations(B + v) = tri(B[S]) + ind5(B[V(B) \ S])

where S = N(v) is the neighbourhood of the new vertex v.  So:

  * enumerating all valid 35-vertex (4,6)-graphs is an *easy* SAT problem
    (595 variables) -- our solver finds the known ones in seconds;
  * for a fixed 35-graph B, asking "does some S work?" is a SAT problem with
    only |V| = 35 Boolean variables -- instant.

Hence:  if we enumerate every 35-graph and none of them extends, then no
36-vertex (4,6)-graph exists, i.e. R(4,6) = 36.  Conversely, if one of them
extends, we have a 36-vertex graph, which would beat the board record.

Usage
-----
  python3 enum35.py <work.cnf> <outdir> <max_solutions> <secs_per_solve> [solver]

The base CNF must be the 35-vertex instance (NVERT=35 ./sat36 base.cnf).
Each solution is written to <outdir>/g_<i>.json as {"adjacency": [...]}, and
immediately tested for 36-vertex extendability.  A SAT extension is written
to <outdir>/EXT_<i>.json and is a world-record candidate.
"""
import json
import os
import subprocess
import sys
import time


def run_solver(cnf, secs, solver):
    """Run the SAT solver on cnf; return ('SAT', assignment) | ('UNSAT', None)
    | ('UNKNOWN', None).  assignment maps var -> bool for the edge variables."""
    # NOTE 1: kissat's --time is in SECONDS, and it only parses INTEGERS --
    # "--time=120.000" makes it print a usage error and exit 1 (which we used
    # to misread as UNKNOWN).
    # NOTE 2: the adjacency matrix may arrive as strings ("0101...") or bools,
    # so coerce it before any truthiness test.
    cmd = solver + ["--time=%d" % int(secs), cnf]
    try:
        p = subprocess.run(cmd, capture_output=True, text=True, timeout=secs + 30)
    except subprocess.TimeoutExpired:
        return "UNKNOWN", None
    out = p.stdout + p.stderr
    if "s SATISFIABLE" in out or p.returncode == 10:
        vals = {}
        for line in out.splitlines():
            if line.startswith("v "):
                for tok in line.split()[1:]:
                    if tok in ("s", "SATISFIABLE", "UNSATISFIABLE"):
                        continue
                    try:
                        iv = int(tok)
                    except ValueError:
                        continue
                    vals[abs(iv)] = (iv > 0)
        # every edge variable must be present, otherwise the blocking clause
        # we derive from this model would be wrong (and we would silently skip
        # solutions).  kissat prints the full assignment, so this should hold.
        need = set(edge_index(35).keys())
        if not need.issubset(set(vals.keys())):
            print("WARNING: model is missing %d variables -- refusing to use it"
                  % len(need - set(vals.keys())), flush=True)
            return "UNKNOWN", None
        return "SAT", vals
    if "s UNSATISFIABLE" in out or p.returncode == 20:
        return "UNSAT", None
    return "UNKNOWN", None


def edge_index(n):
    """var id -> (i,j) with i<j, in the order sat36.c assigns them."""
    idx = {}
    v = 0
    for i in range(n):
        for j in range(i + 1, n):
            v += 1
            idx[v] = (i, j)
    return idx


def to_graph(n, vals):
    A = [[0] * n for _ in range(n)]
    for v, (i, j) in edge_index(n).items():
        if vals.get(v, False):
            A[i][j] = A[j][i] = 1
    return A


def build_ext_cnf(n, A, path):
    """CNF for: does there exist S = N(v) with B[S] triangle-free and
    B[V \\ S] free of independent 5-sets?  Variables x_i = [i in S]."""
    # normalise: rows may be strings ("0101") or bools
    A = [[(1 if c in (1, True, "1") else 0) for c in row] for row in A]
    clauses = []
    # 1. every triangle {a,b,c} of B forbids S containing all three
    for a in range(n):
        for b in range(a + 1, n):
            if not A[a][b]:
                continue
            for c in range(b + 1, n):
                if A[a][c] and A[b][c]:
                    clauses.append([-(a + 1), -(b + 1), -(c + 1)])
    # 2. every independent 5-set {a..e} of B forbids S disjoint from it
    #    (i.e. at least one of the five must be in S)
    ind5 = []
    for a in range(n):
        for b in range(a + 1, n):
            if A[a][b]:
                continue
            for c in range(b + 1, n):
                if A[a][c] or A[b][c]:
                    continue
                for d in range(c + 1, n):
                    if A[a][d] or A[b][d] or A[c][d]:
                        continue
                    for e in range(d + 1, n):
                        if A[a][e] or A[b][e] or A[c][e] or A[d][e]:
                            continue
                        ind5.append((a, b, c, d, e))
    for (a, b, c, d, e) in ind5:
        clauses.append([a + 1, b + 1, c + 1, d + 1, e + 1])
    with open(path, "w") as f:
        f.write("p cnf %d %d\n" % (n, len(clauses)))
        for cl in clauses:
            f.write(" ".join(str(x) for x in cl) + " 0\n")
    return len(clauses), len(ind5)


def blocking_clause(n, vals):
    """Clause that forbids the exact edge set we just found."""
    cl = []
    for v, (i, j) in edge_index(n).items():
        if vals.get(v, False):
            cl.append(-v)
        else:
            cl.append(v)
    return cl


def main():
    if len(sys.argv) < 5:
        print(__doc__)
        return 1
    cnf, outdir = sys.argv[1], sys.argv[2]
    maxsol = int(sys.argv[3])
    secs = float(sys.argv[4])
    solver = sys.argv[5:] or ["kissat"]
    n = 35
    os.makedirs(outdir, exist_ok=True)

    found_ext = 0
    t0 = time.time()
    for it in range(maxsol):
        st, vals = run_solver(cnf, secs, solver)
        if st == "UNSAT":
            print("ENUM COMPLETE: no more 35-vertex (4,6)-graphs after %d solutions "
                  "(%.1f s)" % (it, time.time() - t0), flush=True)
            json.dump({"complete": True, "solutions": it, "extensions": found_ext},
                      open(os.path.join(outdir, "ENUM.json"), "w"), indent=1)
            return 0
        if st == "UNKNOWN":
            print("UNKNOWN after %d solutions (%.1f s) -- stopping" % (it, time.time() - t0),
                  flush=True)
            json.dump({"complete": False, "solutions": it, "extensions": found_ext},
                      open(os.path.join(outdir, "ENUM.json"), "w"), indent=1)
            return 0
        A = to_graph(n, vals)
        gp = os.path.join(outdir, "g_%d.json" % it)
        json.dump({"adjacency": A}, open(gp, "w"))
        # extend to 36?
        ec = os.path.join(outdir, "ext_%d.cnf" % it)
        ncl, nind5 = build_ext_cnf(n, A, ec)
        st2, vals2 = run_solver(ec, 60, solver)
        tag = "UNSAT"
        if st2 == "SAT":
            found_ext += 1
            tag = "*** EXTENDS TO 36 ***"
            S = [i for i in range(n) if vals2.get(i + 1, False)]
            A36 = [row[:] + [0] for row in A] + [[0] * n]
            for i in S:
                A36[i][n] = A36[n][i] = 1
            json.dump({"adjacency": A36}, open(os.path.join(outdir, "EXT_%d.json" % it), "w"))
        print("sol %d: clauses=%d ind5=%d  ext=%s  (%.1f s)" %
              (it, ncl, nind5, tag, time.time() - t0), flush=True)
        # add the blocking clause for this edge set
        with open(cnf, "a") as f:
            f.write(" ".join(str(x) for x in blocking_clause(n, vals)) + " 0\n")
    json.dump({"complete": False, "solutions": maxsol, "extensions": found_ext},
              open(os.path.join(outdir, "ENUM.json"), "w"), indent=1)
    print("hit the solution cap (%d)" % maxsol, flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
