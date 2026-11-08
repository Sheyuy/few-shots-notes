#!/usr/bin/env python3
"""
validate35.py -- independent validator and WL-dedup for (4,6) graphs.

This is deliberately a *separate implementation* from the search code: the
search's own incremental violation counter is buggy (it can even go negative),
so every candidate must be checked by an independent routine before it counts.

The independent-6 test uses brute-force enumeration of all 5-subsets and then
looks for a vertex that is adjacent to none of them, i.e. exactly the
definition, with no clever pruning that could hide a case.

Usage:
  python3 validate35.py <graphs.json> [--out valid.json] [--known known.json]
"""
import hashlib
import itertools
import json
import sys


def to_int_matrix(raw):
    return [[int(c) for c in row] for row in raw]


def counts(A):
    """(number of K4s, number of independent 6-sets)"""
    n = len(A)
    adj = [0] * n
    for i in range(n):
        for j in range(n):
            if A[i][j]:
                adj[i] |= 1 << j
    k4 = 0
    for i in range(n):
        nb = [j for j in range(n) if A[i][j]]
        for a in range(len(nb)):
            for b in range(a + 1, len(nb)):
                if not (adj[nb[a]] >> nb[b]) & 1:
                    continue
                for c in range(b + 1, len(nb)):
                    if (adj[nb[a]] >> nb[c]) & 1 and (adj[nb[b]] >> nb[c]) & 1:
                        k4 += 1
    ind6 = 0
    for combo in itertools.combinations(range(n), 5):
        ok = True
        for a in range(5):
            for b in range(a + 1, 5):
                if A[combo[a]][combo[b]]:
                    ok = False
                    break
            if not ok:
                break
        if not ok:
            continue
        m = 0
        for x in combo:
            m |= 1 << x
        for v in range(n):
            if (m >> v) & 1:
                continue
            if (adj[v] & m) == 0:      # v adjacent to none of the five
                ind6 += 1
                break
    return k4, ind6


def wl_hash(A, iters=4):
    n = len(A)
    nb = [frozenset(j for j in range(n) if A[i][j]) for i in range(n)]
    col = [0] * n
    for _ in range(iters):
        col = [hash((tuple(sorted(nb[i])),
                     tuple(sorted(col[j] for j in nb[i])))) for i in range(n)]
    return hashlib.sha256(repr(sorted(col)).encode()).hexdigest()[:16]


def load_any(fn):
    d = json.load(open(fn))
    if isinstance(d, dict) and "adjacency" in d:
        return [to_int_matrix(d["adjacency"])]
    gs = d if isinstance(d, list) else list(d.values())
    out = []
    for g in gs:
        raw = g["adjacency"] if isinstance(g, dict) else g
        out.append(to_int_matrix(raw))
    return out


def main():
    fn = sys.argv[1]
    out_fn = None
    known_fn = None
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == "--out":
            out_fn = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "--known":
            known_fn = sys.argv[i + 1]
            i += 2
        else:
            i += 1

    known = set()
    if known_fn:
        known = set(wl_hash(A) for A in load_any(known_fn))

    graphs = load_any(fn)
    valid, invalid, novel = [], 0, []
    for A in graphs:
        n = len(A)
        if n != 35 or any(len(r) != 35 for r in A):
            invalid += 1
            continue
        k4, i6 = counts(A)
        if k4 or i6:
            invalid += 1
            continue
        h = wl_hash(A)
        valid.append(h)
        if h not in known:
            novel.append(A)
    print("candidates=%d valid=%d invalid=%d novel=%d"
          % (len(graphs), len(valid), invalid, len(novel)))
    if out_fn:
        json.dump([{"adjacency": ["".join(str(c) for c in row) for row in A]}
                   for A in novel], open(out_fn, "w"))
        print("wrote", out_fn)


if __name__ == "__main__":
    main()
