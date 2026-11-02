#!/usr/bin/env python3
"""Near-miss 36-vertex (4,6)-graphs from the 37 known valid 35-vertex graphs.

Reasoning
---------
Let G be a valid 35-vertex (4,6)-graph (0 K4, 0 I6) and add a new vertex v with
neighbourhood S.  Then in the resulting 36-vertex graph:

  #K4  = #triangles of G inside S            (a K4 containing v)
  #I6  = #independent 5-sets of G inside V\\S (an I6 containing v)

So the total violation count of the 36-vertex graph is exactly

  f(S) = tri(G[S]) + ind5(G[V\\S])

The extension SAT is UNSAT for all 37 known G (so f >= 1), but *minimising f*
is a tiny optimisation problem over 35 bits that simulated annealing solves in
milliseconds.  f = 1 gives a 36-vertex graph that is ONE violation away from a
world record, and that is a far better tabu seed than anything we had.

Usage: python3 nearmiss36.py [graph.json] [out.json]
"""
import json
import random
import sys
from itertools import combinations


def build_index(adj):
    """adj: list of bitmask adjacency.

    Return (tri_masks, ind5_masks, all_tri, all_ind5) where tri_masks[v] is the
    list of masks of the *other two* vertices of every triangle through v, and
    ind5_masks[v] the list of masks of the *other four* vertices of every
    independent 5-set through v.
    """
    n = len(adj)
    full = (1 << n) - 1

    all_tri = list(_triangles(adj))
    tri_masks = [[] for _ in range(n)]
    for (a, b, c) in all_tri:
        tri_masks[a].append((1 << b) | (1 << c))
        tri_masks[b].append((1 << a) | (1 << c))
        tri_masks[c].append((1 << a) | (1 << b))

    all_ind5 = list(_ind5(adj))
    ind5_masks = [[] for _ in range(n)]
    for tup in all_ind5:
        tm = 0
        for v in tup:
            tm |= 1 << v
        for v in tup:
            ind5_masks[v].append(tm & ~(1 << v))

    return tri_masks, ind5_masks, all_tri, all_ind5


def f_of(S, adj, tri_by_v, ind5_by_v, n):
    """Direct evaluation (used to verify the incremental version)."""
    tri = 0
    for (a, b, c) in _triangles(adj):
        if (S >> a) & 1 and (S >> b) & 1 and (S >> c) & 1:
            tri += 1
    i5 = 0
    full = (1 << n) - 1
    for tup in _ind5(adj):
        if (full & ~S) & sum(1 << x for x in tup) == sum(1 << x for x in tup):
            i5 += 1
    return tri + i5


def _triangles(adj):
    n = len(adj)
    full = (1 << n) - 1
    for j in range(n):
        gt = ~((1 << (j + 1)) - 1) & full
        for i in range(j):
            if (adj[i] >> j) & 1:
                m = adj[i] & adj[j] & gt
                while m:
                    low = m & -m
                    k = low.bit_length() - 1
                    m ^= low
                    yield (i, j, k)


def _ind5(adj):
    n = len(adj)
    full = (1 << n) - 1
    nonadj = [full & ~adj[i] & ~(1 << i) for i in range(n)]
    out = []

    def rec(mask, chosen):
        if len(chosen) == 5:
            out.append(tuple(chosen))
            return
        while mask:
            low = mask & -m if False else (mask & -mask)
            v = low.bit_length() - 1
            mask ^= low
            rec(mask & nonadj[v], chosen + [v])

    rec(full, [])
    return out


def anneal(adj, iters=30000, seed=0, restarts=8):
    """Simulated annealing over S (bitmask) minimising f(S).

    f(S) = #triangles of G fully inside S  +  #independent 5-sets of G fully
           inside V\\S.
    """
    n = len(adj)
    tri_masks, ind5_masks, all_tri, all_ind5 = build_index(adj)
    rng = random.Random(seed)
    best_overall = None

    def f_direct(S):
        t = 0
        for (a, b, c) in all_tri:
            if (S >> a) & 1 and (S >> b) & 1 and (S >> c) & 1:
                t += 1
        comp = ((1 << n) - 1) & ~S
        i = 0
        for tup in all_ind5:
            m = 0
            for v in tup:
                m |= 1 << v
            if comp & m == m:
                i += 1
        return t + i

    for r in range(restarts):
        S = 0
        for v in range(n):
            if rng.random() < 0.5:
                S |= 1 << v
        cur = f_direct(S)
        T0, T1 = 3.0, 0.02
        for it in range(iters):
            T = T0 * (T1 / T0) ** (it / iters)
            v = rng.randrange(n)
            inS = (S >> v) & 1
            d_tri = 0
            for pm in tri_masks[v]:
                if pm & S == pm:
                    d_tri += 1 if not inS else -1
            d_i5 = 0
            for rm in ind5_masks[v]:
                if rm & S == 0:
                    d_i5 += 1 if inS else -1
            delta = d_tri + d_i5
            if delta <= 0 or rng.random() < pow(2.718281828, -delta / max(T, 1e-9)):
                S ^= 1 << v
                cur += delta
            if best_overall is None or cur < best_overall[0]:
                best_overall = (cur, S)
                if cur == 0:
                    return best_overall
    return best_overall


def check36(adj36):
    """Independent verification: count K4 and I6 in a full adjacency matrix."""
    n = len(adj36)
    k4 = 0
    for a, b, c, d in combinations(range(n), 4):
        if (adj36[a][b] and adj36[a][c] and adj36[a][d]
                and adj36[b][c] and adj36[b][d] and adj36[c][d]):
            k4 += 1
    i6 = 0
    for s in combinations(range(n), 6):
        if not any(adj36[a][b] for a, b in combinations(s, 2)):
            i6 += 1
    return k4, i6


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'exoo35_graphs.json'
    out = sys.argv[2] if len(sys.argv) > 2 else 'nearmiss36_seeds.json'
    graphs = json.load(open(src))
    print(f'{src}: {len(graphs)} graphs', flush=True)

    seeds = []
    summary = []
    for gi, g in enumerate(graphs):
        rows = g['adjacency']
        n = len(rows)
        adj = []
        for i in range(n):
            m = 0
            for j in range(n):
                if i != j and rows[i][j] == '1':
                    m |= 1 << j
            adj.append(m)
        best = anneal(adj, seed=1000 + gi, restarts=4)
        f, S = best
        # build the 36-vertex graph and verify independently
        rows36 = [r + '0' for r in rows]          # new vertex index 35
        for v in range(n):
            if (S >> v) & 1:
                rows36[v] = rows36[v][:n] + '1'
        rows36.append(''.join('1' if (S >> v) & 1 else '0' for v in range(n)))
        k4, i6 = check36(rows36)
        deg = [sum(1 for j in range(36) if i != j and rows36[i][j] == '1')
               for i in range(36)]
        ok = (k4 + i6 == f)
        summary.append((gi, f, k4, i6, ok))
        print(f'g{gi:02d}: f(S)={f}  verified K4={k4} I6={i6} total={k4+i6} '
              f'match={ok} deg=[{min(deg)},{max(deg)}]', flush=True)
        seeds.append({'adjacency': rows36, 'f': f, 'k4': k4, 'i6': i6})

    json.dump(seeds, open(out, 'w'))
    best = min(summary, key=lambda x: x[1])
    print(f'--- best: g{best[0]} f={best[1]} (K4={best[2]} I6={best[3]}) -> {out}',
          flush=True)


if __name__ == '__main__':
    main()
