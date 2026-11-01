#!/usr/bin/env python3
"""Extension SAT: can a valid 35-vertex (4,6)-graph be extended to 36 vertices?

For a valid 35-vertex (4,6)-graph B and a new vertex v with neighbourhood S:

    B + v is a valid 36-vertex (4,6)-graph
      <=>  B[S] has no triangle            (else v + triangle = K4)
      <=>  B[V\\S] has no independent 5-set  (else v + that set = I6)

So with x_i = "i is in S":

    for every triangle T={a,b,c} of B :  clause  (~x_a | ~x_b | ~x_c)
    for every independent 5-set A of B :  clause  (x_a | x_b | x_c | x_d | x_e)

35 variables, ~1200 clauses -- an exact decision, no heuristics involved.
This is the decisive test for R(4,6) >= 37: a 35-vertex graph B that is
extendable IS a 36-vertex (4,6)-graph.

Usage: python3 ext_sat.py <graphs.txt> [solver] [out.json]
   graphs.txt : one graph per line, "n m0 m1 ..." with hex adjacency masks
   solver     : path to a minisat-compatible solver (default ./minisat_bin)
   out.json   : where to write the 36-vertex certificates
"""
import json
import os
import subprocess
import sys
from itertools import combinations


def read_graphs(path):
    out = []
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                continue
            n = int(p[0])
            out.append([int(x, 16) for x in p[1:1 + n]])
    return out


def triangles(G):
    n = len(G)
    out = []
    for a, b, c in combinations(range(n), 3):
        if (G[a] >> b) & 1 and (G[a] >> c) & 1 and (G[b] >> c) & 1:
            out.append((a, b, c))
    return out


def ind5(G):
    """All independent 5-sets (DFS over non-neighbours)."""
    n = len(G)
    full = (1 << n) - 1
    nonadj = [full & ~G[i] & ~(1 << i) for i in range(n)]
    out = []

    def rec(cand, chosen):
        if len(chosen) == 5:
            out.append(tuple(chosen))
            return
        while cand:
            lo = cand & -cand
            v = lo.bit_length() - 1
            cand ^= lo
            chosen.append(v)
            rec(cand & nonadj[v], chosen)
            chosen.pop()

    rec(full, [])
    return out


def build_cnf(G):
    n = len(G)
    tri = triangles(G)
    i5 = ind5(G)
    clauses = []
    for (a, b, c) in tri:
        clauses.append([-(a + 1), -(b + 1), -(c + 1)])
    for tup in i5:
        clauses.append([v + 1 for v in tup])
    return clauses, tri, i5


def solve(clauses, nvars, solver, tmpdir):
    cnf = os.path.join(tmpdir, 'ext.cnf')
    sat = os.path.join(tmpdir, 'ext.sat')
    with open(cnf, 'w') as f:
        f.write(f"p cnf {nvars} {len(clauses)}\n")
        for c in clauses:
            f.write(' '.join(str(x) for x in c) + ' 0\n')
    r = subprocess.run([solver, cnf, sat], capture_output=True, text=True, timeout=300)
    if not os.path.exists(sat):
        return None, r.stdout[-500:] + r.stderr[-500:]
    model = {}
    unsat = False
    for line in open(sat):
        line = line.strip()
        if not line:
            continue
        if line.startswith('UNSAT'):
            unsat = True
            continue
        if line.startswith('SAT'):
            continue
        if line.startswith('s'):
            if 'UNSAT' in line:
                unsat = True
            continue
        if line.startswith('v'):
            line = line[1:]
        for tok in line.split():
            if not tok.lstrip('-').isdigit():
                continue
            v = int(tok)
            if v == 0:
                break
            model[abs(v)] = v > 0
    if unsat:
        return 'UNSAT', ''
    if nvars not in model:
        return None, 'model incomplete'
    return model, ''


def build36(G, S):
    n = len(G)
    H = [[0] * (n + 1) for _ in range(n + 1)]
    for i in range(n):
        for j in range(n):
            if (G[i] >> j) & 1:
                H[i][j] = 1
    for i in range(n):
        if (S >> i) & 1:
            H[i][n] = 1
            H[n][i] = 1
    return H


def score(H):
    n = len(H)
    full = (1 << n) - 1
    adj = [0] * n
    for i in range(n):
        m = 0
        for j in range(n):
            if H[i][j]:
                m |= 1 << j
        adj[i] = m
    k4 = 0
    for j in range(n):
        gt = ~((1 << (j + 1)) - 1) & full
        for i in range(j):
            if (adj[i] >> j) & 1:
                c = adj[i] & adj[j] & gt
                while c:
                    lo = c & (-c)
                    v = lo.bit_length() - 1
                    c ^= lo
                    k4 += bin(c & adj[v]).count('1')
    nonadj = [full & ~adj[i] & ~(1 << i) for i in range(n)]

    def count(mask, size):
        if size == 0:
            return 1
        total = 0
        while mask:
            lo = mask & (-mask)
            v = lo.bit_length() - 1
            mask ^= lo
            total += count(mask & nonadj[v], size - 1)
        return total

    return k4, count(full, 6)


def main():
    gfile = sys.argv[1]
    solver = sys.argv[2] if len(sys.argv) > 2 else './minisat_bin'
    outjson = sys.argv[3] if len(sys.argv) > 3 else None
    graphs = read_graphs(gfile)
    print(f'{gfile}: {len(graphs)} graphs', flush=True)
    tmpdir = os.path.dirname(os.path.abspath(gfile))
    sat_hits = []
    unsat = 0
    for gi, G in enumerate(graphs):
        clauses, tri, i5 = build_cnf(G)
        model, err = solve(clauses, len(G), solver, tmpdir)
        if model == 'UNSAT':
            unsat += 1
            continue
        if model is None:
            print(f'  g{gi}: SOLVER ERROR {err[:200]}', flush=True)
            continue
        S = 0
        for v in range(1, len(G) + 1):
            if model.get(v):
                S |= 1 << (v - 1)
        H = build36(G, S)
        k4, i6 = score(H)
        if k4 == 0 and i6 == 0:
            tag = f'g{gi}'
            print(f'*** g{gi}: SATISFIABLE -> valid 36-vertex graph '
                  f'(K4={k4} I6={i6})', flush=True)
            if outjson:
                adj = [''.join(map(str, r)) for r in H]
                fn = f'{outjson.rsplit(".", 1)[0]}_{tag}.json'
                json.dump({'adjacency': adj}, open(fn, 'w'))
                print(f'    -> {fn}', flush=True)
            sat_hits.append({'g': gi, 'S': S, 'file': fn if outjson else None})
        else:
            unsat += 1
        if (gi + 1) % 100 == 0:
            print(f'  ... {gi+1}/{len(graphs)}  sat={len(sat_hits)}', flush=True)
    print(f'--- SAT(36-vertex)={len(sat_hits)}  UNSAT={unsat}', flush=True)
    if outjson:
        json.dump(sat_hits, open(outjson, 'w'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
