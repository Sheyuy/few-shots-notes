#!/usr/bin/env python3
"""
merge_new35.py -- verify and merge newly found 35-vertex (4,6)-counterexamples
into the base pool.

Every candidate is checked with TWO independent implementations before it is
allowed into the pool:

  1. validate35.counts()  -- enumerate all C(35,5) subsets and look for a
     vertex adjacent to none of them (plus a K4 count)
  2. exhaustive_check()  -- enumerate all C(36,6) subsets directly

and is then deduplicated with a 4-round Weisfeiler-Lehman hash against the
pool.  Only what survives all of that is added.

Usage:
  python3 merge_new35.py <pool.json> <candidate.json> [<candidate.json> ...]
"""
import itertools
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from validate35 import counts, wl_hash   # noqa: E402


def load_any(fn):
    d = json.load(open(fn))
    if isinstance(d, dict) and "adjacency" in d:
        return [[[int(c) for c in r] for r in d["adjacency"]]]   # one matrix
    gs = d if isinstance(d, list) else list(d.values())
    out = []
    for g in gs:
        raw = g["adjacency"] if isinstance(g, dict) else g
        out.append([[int(c) for c in r] for r in raw])
    return out


def exhaustive_check(A):
    """Third implementation: brute force over all 4-subsets and 6-subsets."""
    n = len(A)
    adj = [0] * n
    for i in range(n):
        for j in range(n):
            if A[i][j]:
                adj[i] |= 1 << j
    k4 = 0
    for q in itertools.combinations(range(n), 4):
        if all((adj[a] >> b) & 1 for a in q for b in q if a < b):
            k4 += 1
    i6 = 0
    for q in itertools.combinations(range(n), 6):
        ok = True
        for a in range(6):
            for b in range(a + 1, 6):
                if (adj[q[a]] >> q[b]) & 1:
                    ok = False
                    break
            if not ok:
                break
        if ok:
            i6 += 1
    return k4, i6


def main():
    pool_file = sys.argv[1]
    pool = json.load(open(pool_file))
    pool_mats = []
    for g in pool:
        raw = g["adjacency"] if isinstance(g, dict) else g
        pool_mats.append([[int(c) for c in r] for r in raw])
    known = set(wl_hash(A) for A in pool_mats)

    added, rejected, dup = 0, 0, 0
    for fn in sys.argv[2:]:
        for A in load_any(fn):
            if len(A) != 35 or any(len(r) != 35 for r in A):
                rejected += 1
                continue
            c1 = counts(A)
            c2 = exhaustive_check(A)
            if c1 != (0, 0) or c2 != (0, 0):
                rejected += 1
                continue
            h = wl_hash(A)
            if h in known:
                dup += 1
                continue
            known.add(h)
            pool.append({"adjacency": ["".join(str(c) for c in r) for r in A]})
            added += 1
    json.dump(pool, open(pool_file, "w"))
    print("pool: %d graphs (+%d new, %d rejected, %d duplicates)"
          % (len(pool), added, rejected, dup))


if __name__ == "__main__":
    main()
