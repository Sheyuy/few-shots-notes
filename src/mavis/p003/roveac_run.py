#!/usr/bin/env python3
"""
roveac_run.py -- run Lehavi's ROVEaC (Ramsey One-Vertex Extension and Checking)
on our own base set of (4,6,35) counterexamples.

Why
---
Lehavi (arXiv:2411.04267) proved that R(4,6,36) is empty *given the 37 known*
R(4,6,35) counterexamples, using an exhaustive one-vertex-extension algorithm.
His Algorithm 1 also shows the sharp necessary condition: if a 36-vertex
counterexample exists, then *all* of its 36 vertex-deleted subgraphs must be
counterexamples.  So the only way to a 36-vertex graph is to know more 35-vertex
counterexamples.

We have 62 valid 35-vertex (4,6)-graphs: the standard 37 (Exoo 2012) plus 25
more that we found (WL-verified non-isomorphic to the standard 37).  This
script runs his exhaustive extension over:

  A) the standard 37            -- must reproduce his "empty" result
  B) our 62                     -- strictly stronger coverage
  C) the decrement-and-reconstruct route (his section 5.3)

Any non-empty result in (B) or (C) is a 36-vertex (4,6)-graph, i.e. a world
record for p003 (the board record is 35 vertices).

Usage:  python3 roveac_run.py <exoo35.json> <pool35.json>
"""
import json
import sys
import time

import networkx as nx

from roveac import Extender
from roveac.decrementor import Decrementor

OPTS = dict(
    extension_method="ove",
    mapping_constructor_method="triangle",
    hash_method="triangle",
    check_method="subgraph_st",
    generate_key_method="triangle",
)


def load_adj(fn):
    d = json.load(open(fn))
    gs = d if isinstance(d, list) else list(d.values())
    return [[[int(c) for c in row] for row in g["adjacency"]] for g in gs]


def to_nx(A):
    # Build the graph with plain networkx: the runner's Python 3.14 has no
    # numpy, and nx.from_numpy_array needs numpy *and* a bool dtype.  Two
    # separate remote runs died on that; this needs neither.
    n = len(A)
    g = nx.Graph()
    g.add_nodes_from(range(n))
    g.add_edges_from((i, j) for i in range(n) for j in range(i + 1, n) if A[i][j])
    return g


def wl_key(A, iters=4):
    import hashlib
    n = len(A)
    nb = [frozenset(j for j in range(n) if A[i][j]) for i in range(n)]
    col = [0] * n
    for _ in range(iters):
        col = [hash((tuple(sorted(nb[i])),
                     tuple(sorted(col[j] for j in nb[i])))) for i in range(n)]
    return hashlib.sha256(repr(sorted(col)).encode()).hexdigest()[:16]


def report(tag, res, t0):
    print("[%s] %d graph(s) found in %.1f s" % (tag, len(res), time.time() - t0),
          flush=True)
    for i, g in enumerate(res):
        A = nx.to_numpy_array(g)
        print("   candidate %d: n=%d  edges=%d" % (i, A.shape[0], int(A.sum() // 2)),
              flush=True)
        # keep EVERY candidate: the previous version overwrote one file and we
        # lost the whole set except the last one.
        json.dump({"adjacency": [[int(x) for x in row] for row in A]},
                  open("FOUND_%s_%03d.json" % (tag, i), "w"))
    return len(res)


def main():
    exoo = load_adj(sys.argv[1])
    pool = load_adj(sys.argv[2])
    kh = set(wl_key(A) for A in exoo)
    new = [A for A in pool if wl_key(A) not in kh]
    print("standard 37: %d graphs | our extra 35-graphs: %d | total: %d"
          % (len(exoo), len(new), len(exoo) + len(new)), flush=True)

    # A) reproduction check: the standard 37 must give an empty set
    t0 = time.time()
    r37 = Extender.extend({to_nx(A) for A in exoo}, s=4, t=6, **OPTS)
    n37 = report("A_standard37", r37, t0)

    # B) our extended base set of 62
    t0 = time.time()
    r62 = Extender.extend({to_nx(A) for A in exoo + new}, s=4, t=6, **OPTS)
    n62 = report("B_ours62", r62, t0)

    # C) decrement-and-reconstruct (his section 5.3)
    t0 = time.time()
    try:
        dec = Decrementor.decrement({to_nx(A) for A in exoo + new},
                                    method="triangle")
        print("[C_decrement] decremented set size: %d (%.1f s)"
              % (len(dec), time.time() - t0), flush=True)
        rec = Extender.extend(dec, s=4, t=6, **OPTS)
        ndec = report("C_decrement", rec, t0)
    except Exception as e:
        ndec = -1
        print("[C_decrement] failed: %r" % (e,), flush=True)

    summary = {"A_standard37": n37, "B_ours62": n62, "C_decrement": ndec,
               "extra_bases": len(new), "total_bases": len(exoo) + len(new)}
    json.dump(summary, open("ROVEAC_SUMMARY.json", "w"), indent=1)
    print("SUMMARY " + json.dumps(summary), flush=True)
    if n62 > 0 or ndec > 0:
        print("*** A 36-VERTEX (4,6)-GRAPH EXISTS -- WORLD RECORD ***", flush=True)


if __name__ == "__main__":
    main()
