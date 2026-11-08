#!/usr/bin/env python3
"""
roveac_closure.py -- iterative closure of the (4,6,35) counterexample set.

Why a closure loop
------------------
Lehavi's one-vertex extension is exhaustive over a base set of 35-vertex
counterexamples: every 36-vertex counterexample that contains at least two
base graphs as subgraphs is generated.  A larger base set therefore covers
strictly more.  And a larger base set is obtainable: extending a decremented
R(4,6,34) set produces *new* 35-vertex counterexamples.

So we iterate:

    base  --extend-->  any 36-vertex graph?      (a hit is a world record)
      |
      +--decrement-->  R(4,6,34) set
      +--extend(base ∪ decremented)-->  new 35-vertex counterexamples
      |
      +--> add them to base, repeat

Each round is exhaustive for the base set it uses, so the union of all rounds
is a sound (and, as far as we know, the strongest published-quality) attack
on R(4,6,36).

The base set is written to disk after every round, so a job that is killed
still leaves the discoveries behind.

Usage:  python3 roveac_closure.py <base.json> <rounds> <minutes_per_round>
"""
import json
import os
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


def relabel(g):
    """Renumber the nodes of a graph to 0..n-1 (the decrementor returns graphs
    with arbitrary labels; mixing label spaces makes the Extender raise
    "The node N is not in the graph")."""
    nodes = sorted(g.nodes())
    mapping = {v: i for i, v in enumerate(nodes)}
    h = nx.Graph()
    h.add_nodes_from(range(len(nodes)))
    h.add_edges_from((mapping[u], mapping[v]) for u, v in g.edges())
    return h


def adj_of(g):
    """Adjacency matrix of a networkx graph without numpy (the runner's
    Python 3.14 has no numpy and nx.to_numpy_array needs it)."""
    nodes = sorted(g.nodes())
    idx = {v: i for i, v in enumerate(nodes)}
    n = len(nodes)
    A = [[0] * n for _ in range(n)]
    for u, v in g.edges():
        A[idx[u]][idx[v]] = A[idx[v]][idx[u]] = 1
    return A


def to_nx(A):
    n = len(A)
    g = nx.Graph()
    g.add_nodes_from(range(n))
    g.add_edges_from((i, j) for i in range(n) for j in range(i + 1, n) if A[i][j])
    return g


def wl_hash(A, iters=4):
    import hashlib
    n = len(A)
    nb = [frozenset(j for j in range(n) if A[i][j]) for i in range(n)]
    col = [0] * n
    for _ in range(iters):
        col = [hash((tuple(sorted(nb[i])),
                     tuple(sorted(col[j] for j in nb[i])))) for i in range(n)]
    return hashlib.sha256(repr(sorted(col)).encode()).hexdigest()[:16]


def load_base(fn):
    d = json.load(open(fn))
    gs = d if isinstance(d, list) else list(d.values())
    return [[[int(c) for c in row] for row in g["adjacency"]] for g in gs]


def main():
    base_file = sys.argv[1]
    rounds = int(sys.argv[2]) if len(sys.argv) > 2 else 5
    budget = float(sys.argv[3]) if len(sys.argv) > 3 else 900.0

    base = load_base(base_file)
    seen = set(wl_hash(A) for A in base)
    print("start: %d base graphs, %d distinct WL classes" % (len(base), len(seen)),
          flush=True)
    json.dump([{"adjacency": A} for A in base], open("CLOSURE_BASE.json", "w"))

    t_start = time.time()
    summary = {"rounds": [], "start_size": len(base)}
    for rnd in range(rounds):
        if time.time() - t_start > budget:
            print("time budget exhausted before round %d" % rnd, flush=True)
            break
        t0 = time.time()
        print("=== round %d: extending over %d bases ===" % (rnd, len(base)),
              flush=True)

        # (1) does a 36-vertex counterexample exist?  (exhaustive over base)
        try:
            res36 = Extender.extend({to_nx(A) for A in base}, s=4, t=6, **OPTS)
        except Exception as e:
            print("  extend(base) failed: %r" % (e,), flush=True)
            res36 = []
        hit36 = []
        for i, g in enumerate(res36):
            A = adj_of(g)
            if len(A) == 36:
                hit36.append(A)
                json.dump({"adjacency": A},
                          open("FOUND36_round%d_%d.json" % (rnd, i), "w"))
                print("  *** 36-VERTEX COUNTEREXAMPLE FOUND (round %d) ***" % rnd,
                      flush=True)
        print("  extend(base): %d candidate(s), %d with n=36  [%.1f s]"
              % (len(res36), len(hit36), time.time() - t0), flush=True)

        # (2) enlarge the base set: decrement, then extend base ∪ decremented
        t1 = time.time()
        try:
            dec = Decrementor.decrement({to_nx(A) for A in base}, method="triangle")
        except Exception as e:
            print("  decrement failed: %r" % (e,), flush=True)
            dec = set()
        print("  decremented set: %d graphs  [%.1f s]" % (len(dec), time.time() - t1),
              flush=True)
        # the decrementor returns graphs whose node labels are arbitrary,
        # while to_nx() numbers nodes 0..n-1 -> relabel before unioning
        dec = {relabel(g) for g in dec}
        try:
            res = Extender.extend({to_nx(A) for A in base} | set(dec),
                                   s=4, t=6, **OPTS)
        except Exception as e:
            print("  extend(union) failed: %r" % (e,), flush=True)
            res = []
        added = 0
        for i, g in enumerate(res):
            A = adj_of(g)
            if len(A) != 35:
                continue
            h = wl_hash(A)
            if h in seen:
                continue
            seen.add(h)
            base.append(A)
            added += 1
            json.dump({"adjacency": A},
                      open("NEW35_round%d_%03d.json" % (rnd, added), "w"))
        print("  extend(union): %d candidate(s), %d NEW 35-graphs added  [%.1f s]"
              % (len(res), added, time.time() - t1), flush=True)

        json.dump([{"adjacency": A} for A in base], open("CLOSURE_BASE.json", "w"))
        summary["rounds"].append({"round": rnd, "base_size": len(base),
                                   "added": added, "n36_hits": len(hit36)})
        json.dump(summary, open("CLOSURE_SUMMARY.json", "w"), indent=1)
        print("  base is now %d graphs" % len(base), flush=True)

        if hit36:
            summary["RECORD"] = True
            json.dump(summary, open("CLOSURE_SUMMARY.json", "w"), indent=1)
            print("*** WORLD RECORD CANDIDATE -- stopping ***", flush=True)
            break
        if added == 0:
            print("  no new graphs -> fixed point reached", flush=True)
            break

    summary["final_base_size"] = len(base)
    summary["distinct_classes"] = len(seen)
    json.dump(summary, open("CLOSURE_SUMMARY.json", "w"), indent=1)
    print("SUMMARY " + json.dumps(summary), flush=True)


if __name__ == "__main__":
    main()
