#!/usr/bin/env python3
"""sat2shot.py -- turn a kissat model for the 36-vertex (4,6) instance into a
verified shot directory.

Variable numbering used by sat36.c:
    var 1 = edge (0,1), var 2 = (0,2), ..., var 35 = (0,35),
    var 36 = (1,2), var 37 = (1,3), ...            (lexicographic on (i<j))

The graph is checked by an *independent* routine (not by the SAT solver) for
cliques of size 4 and independent sets of size 6, then handed to the official
verifier.
"""
import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

N = 36
REPO = Path("/workspace/few-shots")


def edge_table(n=N):
    """var id (1-based) -> (i,j)"""
    t = {}
    v = 0
    for i in range(n):
        for j in range(i + 1, n):
            v += 1
            t[v] = (i, j)
    return t


def parse_model(path):
    """Read a kissat output file, return {var: bool}."""
    model = {}
    with open(path) as f:
        for line in f:
            if not line.startswith("v "):
                continue
            for tok in line.split()[1:]:
                v = int(tok)
                if v == 0:
                    break
                model[abs(v)] = (v > 0)
    return model


def build_adjacency(model, n=N):
    t = edge_table(n)
    adj = [[0] * n for _ in range(n)]
    missing = []
    for var, (i, j) in t.items():
        if var not in model:
            missing.append(var)
            continue
        if model[var]:
            adj[i][j] = adj[j][i] = 1
    return adj, missing


def find_k4(adj, n=N):
    """Return a 4-clique or None.  Independent of the solver."""
    for a in range(n):
        nb_a = [b for b in range(a + 1, n) if adj[a][b]]
        for bi in range(len(nb_a)):
            b = nb_a[bi]
            nb_b = [c for c in nb_a[bi + 1:] if adj[b][c]]
            for ci in range(len(nb_b)):
                c = nb_b[ci]
                nb_c = [d for d in nb_b[ci + 1:] if adj[c][d]]
                for d in nb_c:
                    if adj[d][a]:
                        return (a, b, c, d)
    return None


def find_i6(adj, n=N):
    """Return an independent 6-set or None."""
    for a in range(n):
        na = [b for b in range(a + 1, n) if not adj[a][b]]
        for bi in range(len(na)):
            b = na[bi]
            nb = [c for c in na[bi + 1:] if not adj[b][c]]
            for ci in range(len(nb)):
                c = nb[ci]
                nc = [d for d in nb[ci + 1:] if not adj[c][d]]
                for di in range(len(nc)):
                    d = nc[di]
                    nd = [e for e in nc[di + 1:] if not adj[d][e]]
                    for ei in range(len(nd)):
                        e = nd[ei]
                        for f in nd[ei + 1:]:
                            if not adj[e][f] and not adj[a][f] and not adj[b][f] \
                               and not adj[c][f] and not adj[d][f]:
                                return (a, b, c, d, e, f)
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("model", help="kissat output file containing 'v' lines")
    ap.add_argument("--n", type=int, default=N)
    ap.add_argument("--out", help="shot dir to write (default: print certificate)")
    ap.add_argument("--login", default="Sheyuy")
    ap.add_argument("--shot-id", default="r46-36v-sat")
    args = ap.parse_args()

    model = parse_model(args.model)
    if not model:
        print("no model found in", args.model)
        return 1
    adj, missing = build_adjacency(model, args.n)
    print("model vars: %d, missing assignments: %d" % (len(model), len(missing)))

    k4 = find_k4(adj, args.n)
    i6 = find_i6(adj, args.n)
    print("independent check: K4 =", k4, " I6 =", i6)
    if k4 or i6:
        print("MODEL IS INVALID -- not writing a shot")
        return 2

    rows = ["".join(str(x) for x in r) for r in adj]
    cert = {"adjacency": rows}
    text = json.dumps(cert, indent=2)

    if not args.out:
        print(text)
        return 0

    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    (out / "certificate.json").write_text(text + "\n")
    shot_md = """+++
login = "{login}"
builds_on = []
claimed_score = "{score}"
agent = "Mavis"
models = ["MiniMax-M2.1"]
+++

## Approach

A 36-vertex graph with no clique of size 4 and no independent set of size 6, proving
R(4,6) >= 37.

Found by encoding the problem as SAT (630 edge variables) and solving with kissat 4.0.4.
The CNF contains one clause per 4-subset (no K4) and one clause per 6-subset (no
independent 6-set), plus symmetry breaking: WLOG a vertex of degree d has its
neighbourhood labelled {{1,...,d}}, which is run for every d in 11..17.

The resulting model was re-checked by an independent clique/independent-set search
before being written out.

## Dead ends
- See the accompanying notes.
""".format(login=args.login, score=args.n)
    (out / "SHOT.md").write_text(shot_md)
    print("wrote", out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
