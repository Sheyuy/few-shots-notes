#!/usr/bin/env python3
"""Scan induced subgraphs of a candidate (4,6)-graph for smaller valid subgraphs.

For a 36-vertex candidate with K4/I6 violations, it is very informative to know
whether some 35-vertex (or smaller) induced subgraph is already a *valid*
(4,6)-Ramsey graph.  That would (a) prove we are sitting next to a known
35-vertex solution, and (b) give a much better seed for local search.

Usage: python3 sub_scan.py <graph.json> [min_n]
"""
import json
import sys
from itertools import combinations


def violations(adj, idx):
    """Count K4 cliques and independent 6-sets inside the induced subgraph."""
    sub = [adj[i] for i in idx]
    n = len(idx)
    k4 = 0
    for a, b, c, d in combinations(range(n), 4):
        if (sub[a][b] == '1' and sub[a][c] == '1' and sub[a][d] == '1'
                and sub[b][c] == '1' and sub[b][d] == '1' and sub[c][d] == '1'):
            k4 += 1
    i6 = 0
    for s in combinations(range(n), 6):
        if all(sub[a][b] == '0' for a, b in combinations(s, 2)):
            i6 += 1
    return k4, i6


def main():
    path = sys.argv[1]
    min_n = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    adj = json.load(open(path))['adjacency']
    n = len(adj)
    print(f'file={path} n={n} min_n={min_n}')

    # full graph
    k4, i6 = violations(adj, range(n))
    print(f'  n={n}: K4={k4} I6={i6} total={k4+i6}')

    best = None
    for m in range(n - 1, min_n - 1, -1):
        found = []
        counts = {}
        for drop in combinations(range(n), n - m):
            idx = [i for i in range(n) if i not in drop]
            a, b = violations(adj, idx)
            counts[(a, b)] = counts.get((a, b), 0) + 1
            if a == 0 and b == 0:
                found.append(drop)
        print(f'  n={m}: valid({k4=}) count={len(found)}  dist={sorted(counts.items())[:8]}')
        if found:
            print(f'    *** VALID {m}-vertex subgraph(s): {len(found)} of them')
            print(f'    example drop={found[0]}')
            best = (m, found)
            break
    if best is None:
        print('  no valid subgraph down to min_n')


if __name__ == '__main__':
    main()
