import json
import sys

BASE = '/workspace/few-shots/problems/p003-ramsey-r46/baseline/certificate.json'
N = 35


def load():
    with open(BASE) as f:
        rows = json.load(f)['adjacency']
    adj = [int(r[::-1], 2) for r in rows]
    full = (1 << N) - 1
    comp = [full & ~a & ~(1 << i) for i, a in enumerate(adj)]
    return adj, comp


def bits(x):
    while x:
        low = x & -x
        yield low.bit_length() - 1
        x ^= low


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


if __name__ == '__main__':
    adj, comp = load()
    print('edges', sum(bin(a).count('1') for a in adj) // 2)
    tri = all_cliques(adj, 3)
    print('triangles', len(tri))
    ind5 = all_cliques(comp, 5)
    print('independent 5-sets', len(ind5))
    ind6 = all_cliques(comp, 6)
    print('independent 6-sets', len(ind6))
    k4 = all_cliques(adj, 4)
    print('cliques4', len(k4))