#!/usr/bin/env python3
"""Iterative near-miss pipeline for p003 (R(4,6) >= 37).

Core idea
---------
For a valid (4,6)-graph B on m vertices and a new vertex v with neighbourhood S:

    violations(B + v) = tri(B[S]) + ind5(B[V\\S])   ==:  f_B(S)

* f_B(S) = 0  <=>  B + v is a valid (m+1)-vertex (4,6)-graph.
* The extension SAT is UNSAT for the 37 *known* 35-vertex graphs, so f >= 1
  for those.  But the space of valid 35-vertex graphs is far bigger than the
  37 known ones.

Pipeline (iterate until a 36-vertex graph appears):
    gen 0 : the 37 known valid 35-vertex graphs
    step  : for every graph B in the current generation, and every vertex u,
            minimise f_{B\\u}(S).  f = 0 gives a *new valid 35-vertex graph*
            (a "sibling" of B).
    step  : minimise f_B(S) for every graph B in the generation.  f = 0 gives a
            valid 36-vertex graph  ==  R(4,6) >= 37  ==  WORLD RECORD.

Everything is re-verified by an independent Python counter (K4 / I6) that does
not share code with the C optimiser.

Usage: python3 pipeline.py [g35.txt] [outdir] [generations]
"""
import json
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


# ------------------------------------------------------------------ I/O
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


def write_graphs(path, graphs):
    with open(path, 'w') as f:
        for G in graphs:
            n = len(G)
            f.write(str(n) + ' ' + ' '.join('%x' % m for m in G) + '\n')


def build(graphs, g, u, S):
    """Return (adjacency matrix) of B\\u + new vertex with neighbourhood S."""
    G = graphs[g]
    n = len(G)
    mapping = [i for i in range(n) if i != u]
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


def to_masks(H):
    n = len(H)
    out = []
    for i in range(n):
        m = 0
        for j in range(n):
            if H[i][j]:
                m |= 1 << j
        out.append(m)
    return out


def score(H):
    """Independent K4 / I6 counter (bitmask DFS)."""
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


def key_of(H):
    return tuple(to_masks(H))


def run_nearmiss(gfile, outjson, iters=20000, restarts=4):
    cmd = [os.path.join(HERE, 'nearmiss'), gfile, outjson, str(iters), str(restarts)]
    r = subprocess.run(cmd, capture_output=True, text=True)
    sys.stderr.write(r.stderr[-4000:])
    return json.load(open(outjson))


# ------------------------------------------------------------------ main
def main():
    src = sys.argv[1] if len(sys.argv) > 1 else 'g35.txt'
    outdir = sys.argv[2] if len(sys.argv) > 2 else 'pipeline_out'
    gens = int(sys.argv[3]) if len(sys.argv) > 3 else 3
    os.makedirs(outdir, exist_ok=True)

    graphs = read_graphs(src)
    print(f'generation 0: {len(graphs)} graphs', flush=True)
    record = []

    for gen in range(gens):
        gfile = os.path.join(outdir, f'gen{gen}.txt')
        write_graphs(gfile, graphs)
        res = run_nearmiss(gfile, os.path.join(outdir, f'nm{gen}.json'))
        entries = [e for e in res if isinstance(e, dict) and 'f' in e]

        # ---- (a) extension of the current graphs: f == 0 => (n+1)-vertex graph
        extended = []
        for e in entries:
            if e['f'] != 0:
                continue
            H, _ = build(graphs, e['g'], e['u'], e['S'])
            k4, i6 = score(H)
            n = len(H)
            if k4 == 0 and i6 == 0:
                extended.append((e, H, n))
            else:
                print(f'  !! f=0 but independent check says K4={k4} I6={i6} '
                      f'(g={e["g"]} u={e["u"]}) -- BUG', flush=True)

        for e, H, n in extended:
            if n == 36:
                fn = os.path.join(outdir, f'cert36_g{e["g"]}_u{e["u"]}.json')
                json.dump({'adjacency': [''.join(map(str, r)) for r in H]}, open(fn, 'w'))
                print(f'*** 36-VERTEX VALID GRAPH: g={e["g"]} u={e["u"]} -> {fn}',
                      flush=True)
                record.append({'gen': gen, 'g': e['g'], 'u': e['u'], 'n': n})

        # ---- (b) siblings: delete one vertex and re-close => new 35-graphs
        new_graphs = {}
        for e in entries:
            if e['f'] != 0 or e['u'] < 0:
                continue
            H, _ = build(graphs, e['g'], e['u'], e['S'])
            if len(H) != 35:
                continue
            k4, i6 = score(H)
            if k4 == 0 and i6 == 0:
                new_graphs[key_of(H)] = H

        pool = list(new_graphs.values())
        print(f'generation {gen}: {len(entries)} base graphs scanned, '
              f'{len(extended)} valid extensions, {len(pool)} new 35-vertex graphs',
              flush=True)

        if not pool:
            print('no new graphs -- stopping', flush=True)
            break
        graphs = pool

    json.dump(record, open(os.path.join(outdir, 'record.json'), 'w'))
    print('done', flush=True)


if __name__ == '__main__':
    main()
