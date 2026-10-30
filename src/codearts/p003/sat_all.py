import json
import sys
import time

from pysat.solvers import Solver

G6 = '/workspace/few-shots-work/p003/r46_35some.g6'
OUT = '/workspace/few-shots-work/p003/candidate.json'


def parse_g6(s):
    s = s.strip()
    n = ord(s[0]) - 63
    bits = []
    for ch in s[1:]:
        v = ord(ch) - 63
        for k in range(5, -1, -1):
            bits.append((v >> k) & 1)
    adj = [0] * n
    idx = 0
    for j in range(1, n):
        for i in range(j):
            if bits[idx]:
                adj[i] |= 1 << j
                adj[j] |= 1 << i
            idx += 1
    return adj


def all_cliques(nbrs, size):
    n = len(nbrs)
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

    extend([], (1 << n) - 1)
    return out


def build_cnf(adj):
    n = len(adj)
    full = (1 << n) - 1
    comp = [full & ~a & ~(1 << i) for i, a in enumerate(adj)]
    tri = all_cliques(adj, 3)
    ind5 = all_cliques(comp, 5)
    clauses = [[-(v + 1) for v in t] for t in tri]
    clauses += [[(v + 1) for v in s] for s in ind5]
    return clauses, len(tri), len(ind5)


def build_rows(adj, model):
    n = len(adj)
    nbr = 0
    for i in range(n):
        if model[i] > 0:
            nbr |= 1 << i
    rows = []
    for i in range(n):
        r = adj[i]
        if nbr >> i & 1:
            r |= 1 << n
        rows.append(r)
    rows.append(nbr)
    return [format(r, f'0{n+1}b')[::-1] for r in rows]


def main():
    lines = [l for l in open(G6) if l.strip()]
    print('graphs', len(lines), flush=True)
    for idx, line in enumerate(lines):
        adj = parse_g6(line)
        if len(adj) != 35:
            print(idx, 'bad n', len(adj), flush=True)
            continue
        clauses, nt, ni = build_cnf(adj)
        with Solver(name='cadical153', bootstrap_with=clauses) as s:
            ok = s.solve()
            model = s.get_model() if ok else None
        if ok:
            rows = build_rows(adj, model)
            with open(OUT, 'w') as f:
                json.dump({'adjacency': rows}, f)
            print('graph', idx, 'SAT', 'tri', nt, 'ind5', ni, 'SAVED', flush=True)
            return
        print('graph', idx, 'unsat', 'tri', nt, 'ind5', ni, flush=True)
    print('all unsat', flush=True)


if __name__ == '__main__':
    main()