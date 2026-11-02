#!/usr/bin/env python3
"""Rebuild the 36-vertex candidate from (g, u, S) and verify it independently.

Input : g35.txt (one graph per line: "n m0 m1 ...", hex adjacency masks)
        nearmiss_all.json (output of nearmiss.c)
Usage: python3 build_cert.py <g35.txt> <nearmiss.json> [outdir]
        -> writes <outdir>/certificate.json  (format {"adjacency": [...]})
"""
import json
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
            rows = [int(x, 16) for x in p[1:1 + n]]
            out.append(rows)
    return out


def build(graphs, g, u, S):
    """B = G minus vertex u; add new vertex with neighbourhood S (over B)."""
    G = graphs[g]
    n = len(G)
    mapping = [i for i in range(n) if i != u]      # base index -> G index
    m = len(mapping)
    H = [[0] * (m + 1) for _ in range(m + 1)]
    for i in range(m):
        for j in range(m):
            if (G[mapping[i]] >> mapping[j]) & 1:
                H[i][j] = 1
    for i in range(m):
        if (S >> i) & 1:
            H[i][m] = 1
            H[m][i] = 1
    return H, mapping


def score(H):
    """Independent brute-force count of K4 cliques and independent 6-sets.

    Bitmask implementation: C(36,6) = 1.9M combinations in pure Python would
    take minutes, so the independent-6-set count uses a masked DFS.
    """
    n = len(H)
    full = (1 << n) - 1
    adj = [0] * n
    for i in range(n):
        m = 0
        for j in range(n):
            if H[i][j]:
                m |= 1 << j
        adj[i] = m

    # K4: for each edge (i,j), count adjacent pairs among the common neighbours
    k4 = 0
    for j in range(n):
        gt = ~((1 << (j + 1)) - 1) & full
        for i in range(j):
            if (adj[i] >> j) & 1:
                c = adj[i] & adj[j] & gt
                while c:
                    lo = c & (-c & ((1 << 64) - 1))
                    v = lo.bit_length() - 1
                    c ^= lo
                    k4 += bin(c & adj[v]).count('1')

    # I6: masked DFS over independent sets of size 6
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

    i6 = count(full, 6)
    return k4, i6


def main():
    graphs = read_graphs(sys.argv[1])
    data = json.load(open(sys.argv[2]))
    best = data[-1] if isinstance(data[-1], dict) and 'best_f' in data[-1] else None
    entries = [e for e in data if isinstance(e, dict) and 'f' in e]

    # collect all with f == 0 (they are all valid 36-vertex graphs)
    zeros = [e for e in entries if e['f'] == 0]
    print(f'{len(entries)} base graphs scanned; f==0 candidates: {len(zeros)}')
    if best:
        print(f"reported best: g={best['best_g']} u={best['best_u']} f={best['best_f']}")

    if not zeros:
        zs = min(e['f'] for e in entries)
        print(f'no zero candidate; minimum f = {zs}')
        for e in sorted(entries, key=lambda x: x['f'])[:5]:
            print('   ', e)
        return 1

    outdir = sys.argv[3] if len(sys.argv) > 3 else 'cert_candidate'
    import os
    os.makedirs(outdir, exist_ok=True)
    made = 0
    for e in zeros:
        H, mapping = build(graphs, e['g'], e['u'], e['S'])
        k4, i6 = score(H)
        adj = [''.join(str(x) for x in row) for row in H]
        tag = f"g{e['g']}_u{e['u']}"
        if k4 == 0 and i6 == 0:
            fn = f'{outdir}/certificate_{tag}.json'
            json.dump({'adjacency': adj}, open(fn, 'w'))
            print(f'  VALID  {tag}: n={len(adj)} K4={k4} I6={i6} -> {fn}')
            made += 1
        else:
            print(f'  BAD    {tag}: K4={k4} I6={i6} (f said 0!)')
    print(f'--- {made} valid 36-vertex certificates written to {outdir}/')
    return 0


if __name__ == '__main__':
    sys.exit(main())
