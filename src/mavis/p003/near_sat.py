#!/usr/bin/env python3
"""near_sat.py -- SAT with a known 35-vertex (4,6)-graph pinned, with k of its
edges freed, plus one free vertex.

Why this is worth doing
-----------------------
The direct 36-vertex SAT has 630 edge variables and no structure to exploit;
kissat burned 640 million conflicts on it without a verdict.  But we know 37
valid 35-vertex graphs.  If a 36-vertex graph exists, it is plausibly a small
edit of one of them.

So: pin the 35 known edges, free k of them, and leave the 36th vertex's 35
edges completely free.  Then the free variables are k + 35 instead of 630, so
the search space drops from 2^630 to 2^(k+35) and the solver has a fighting
chance.

The k = 0 case is exactly the "can this 35-graph be extended?" question that
extsat.c already answers (UNSAT for all 37 known graphs), so it doubles as a
cross-check on the encoding.

Usage:
  python3 near_sat.py <out.cnf> <graph.json> <k> [--seed S] [--free-vertex]
      graph.json : {"adjacency": [...]} of a 35-vertex valid graph
      k          : how many of the 35 graph's edges to free
      --free-vertex : instead of k random edges, free every edge incident to
                       one chosen vertex (35 edges)
"""
import json
import random
import sys


def main():
    if len(sys.argv) < 4:
        print(__doc__)
        return 1
    out_cnf = sys.argv[1]
    gpath = sys.argv[2]
    k = int(sys.argv[3])
    seed = 12345
    free_vertex = False
    args = sys.argv[4:]
    if "--seed" in args:
        i = args.index("--seed"); seed = int(args[i + 1])
    if "--free-vertex" in args:
        free_vertex = True

    d = json.load(open(gpath))
    adj = d["adjacency"]
    n = len(adj)
    assert n == 35, n
    A = [[int(c) for c in r] for r in adj]

    # relabel so that vertex 0's neighbours are exactly {1..deg0}: the SAT
    # encoding's symmetry break (mode 4, d = deg0) is then consistent with the pin.
    deg0 = sum(A[0])
    nbrs = [i for i in range(1, n) if A[0][i]]
    rest = [i for i in range(1, n) if not A[0][i]]
    perm = [0] + nbrs + rest
    B = [[A[perm[i]][perm[j]] for j in range(n)] for i in range(n)]

    # Choose which pairs to free.  IMPORTANT: B's NON-edges must be freed too,
    # otherwise the pinned instance only covers "supergraphs of B" (and the
    # non-edges stay forced absent) -- a much narrower region than intended,
    # and one that decides instantly for the wrong reason.
    edges = [(i, j) for i in range(n) for j in range(i + 1, n) if B[i][j]]
    nonedges = [(i, j) for i in range(n) for j in range(i + 1, n) if not B[i][j]]
    rng = random.Random(seed)
    freed = set()
    if free_vertex:
        v = rng.randrange(n)
        for (i, j) in edges + nonedges:
            if i == v or j == v:
                freed.add((i, j))
    else:
        # NESTED ladder: shuffle ONCE with the seed and free the first k of each
        # list, so that R(k) subset R(k+1).  Then a single UNSAT at k=K settles
        # every k <= K for this base graph.  (Independent random subsets per k
        # would leave holes in the coverage.)
        eord = edges[:]
        nord = nonedges[:]
        rng.shuffle(eord)
        rng.shuffle(nord)
        for e in eord[:min(k, len(eord))]:
            freed.add(e)
        for e in nord[:min(k, len(nord))]:
            freed.add(e)

    # write the pin file: 35 rows with the freed edges replaced by '-'
    # (the PIN parser skips anything that is not '0'/'1'), plus a 36th row of
    # dashes so that the new vertex's edges are entirely free.
    rows = []
    for i in range(n):
        rows.append("".join("-" if (min(i, j), max(i, j)) in freed else ("1" if B[i][j] else "0")
                            for j in range(n)))
    rows.append("-" * n)
    with open(out_cnf + ".pin", "w") as f:
        f.write("\n".join(rows) + "\n")
    print(f"pin written: {out_cnf}.pin  freed={len(freed)} "
          f"(edges {len(edges)} + non-edges {len(nonedges)}), deg0={deg0} "
          f"mode={'free-vertex' if free_vertex else 'k=%d each way' % k}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
