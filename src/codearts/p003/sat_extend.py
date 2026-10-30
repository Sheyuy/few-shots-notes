import json
import sys
import time

from pysat.solvers import Solver

BASE = '/workspace/few-shots/problems/p003-ramsey-r46/baseline/certificate.json'
N = 35


def load():
    with open(BASE) as f:
        rows = json.load(f)['adjacency']
    adj = [int(r[::-1], 2) for r in rows]
    full = (1 << N) - 1
    comp = [full & ~a & ~(1 << i) for i, a in enumerate(adj)]
    return adj, comp


def all_cliques(nbrs, size):
    out = []

    def extend(chosen, cand):
        if len(chosen) == size:
            out.append(tuple(chosen))
            return
        while cand:
            low = cand & -cand
            v = low.bit_length() - 1
            cand ^= low
            if len(chosen) + 1 + bin(cand).count('1') < size:
                return
            extend(chosen + [v], cand & nbrs[v])

    extend([], (1 << len(nbrs)) - 1)
    return out


def build_cnf(adj, comp):
    tri = all_cliques(adj, 3)
    ind5 = all_cliques(comp, 5)
    clauses = [[-(v + 1) for v in t] for t in tri]
    clauses += [[(v + 1) for v in s] for s in ind5]
    return clauses


def solve(clauses, solver_name='cadical153'):
    with Solver(name=solver_name, bootstrap_with=clauses) as s:
        ok = s.solve()
        model = s.get_model() if ok else None
        return ok, model


def build_certificate(adj, model):
    nbr = 0
    for i in range(N):
        if model[i] > 0:
            nbr |= 1 << i
    rows = []
    for i in range(N):
        r = adj[i]
        if nbr >> i & 1:
            r |= 1 << N
        rows.append(r)
    rows.append(nbr)
    return [format(r, f'0{N+1}b')[::-1] for r in rows]


if __name__ == '__main__':
    adj, comp = load()
    clauses = build_cnf(adj, comp)
    print('clauses', len(clauses), flush=True)
    t = time.time()
    ok, model = solve(clauses)
    print('sat' if ok else 'unsat', 'time', round(time.time() - t, 2), flush=True)
    if ok:
        rows = build_certificate(adj, model)
        for r in rows:
            print(r)
        with open('/workspace/few-shots-work/p003/candidate.json', 'w') as f:
            json.dump({'adjacency': rows}, f)