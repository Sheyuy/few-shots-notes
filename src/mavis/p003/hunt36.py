#!/usr/bin/env python3
"""
hunt36.py -- direct search for a 36-vertex (4,6)-graph (the world record).

Why this is different from hunt35.py
-----------------------------------
Our SAT extension test proves that none of our 35-vertex bases *extends* to 36
vertices.  But the tabu search is not restricted to extensions: it may flip
any edge of the whole graph.  Starting from a valid 35-vertex (4,6)-graph plus
one isolated vertex, the search is already at ~16 violations instead of the
~43 it reaches from a random start -- the landscape around a valid counter-
example is much friendlier.

So this is a genuine direct search for the record, with the same safety net as
every other step: the search's own violation counter is ignored and each
candidate is re-checked by an independent brute-force validator.

A hit is written as FOUND36.json ({"adjacency": [36 strings of 36 chars]}).

Usage:
  python3 hunt36.py <pool.json> <outdir> <n_trials> <secs_per_run> [seed]
"""
import json
import os
import random
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from validate35 import counts   # noqa: E402

P003R = "./p003r"
N = 36


def rows_to_masks(rows, n):
    masks = [0] * n
    for i in range(n):
        for j in range(n):
            if i != j and rows[i][j] == "1":
                masks[i] |= 1 << j
    return masks


def masks_to_rows(masks, n):
    return ["".join("1" if (masks[i] >> j) & 1 else "0" for j in range(n))
            for i in range(n)]


def perturb(masks, rng, k):
    n = len(masks)
    out = list(masks)
    for _ in range(k):
        i, j = rng.randrange(n), rng.randrange(n)
        if i == j:
            continue
        if (out[i] >> j) & 1:
            out[i] &= ~(1 << j)
            out[j] &= ~(1 << i)
        else:
            out[i] |= 1 << j
            out[j] |= 1 << i
    return out


def main():
    pool_file, outdir = sys.argv[1], sys.argv[2]
    trials = int(sys.argv[3])
    secs = float(sys.argv[4])
    seed0 = int(sys.argv[5]) if len(sys.argv) > 5 else 20261108

    os.makedirs(outdir, exist_ok=True)
    d = json.load(open(pool_file))
    pool = [g["adjacency"] for g in d]
    print("base pool: %d graphs (n=35) -> searching n=36" % len(pool), flush=True)

    rng = random.Random(seed0)
    stats = {"trials": 0, "valid": 0, "invalid": 0, "hits": 0}
    best_viol = None
    t0 = time.time()
    for t in range(trials):
        bi = rng.randrange(len(pool))
        k = rng.choice([2, 4, 8, 12, 16, 24, 32, 48, 64])
        # 35-char rows have to grow to 36, and the new vertex starts isolated
        base_rows = [r + "0" for r in pool[bi]] + ["0" * N]
        masks = perturb(rows_to_masks(base_rows, N), rng, k)
        rows = masks_to_rows(masks, N)
        seedf = os.path.join(outdir, "seed36.json")
        json.dump({"adjacency": rows}, open(seedf, "w"))
        outf = os.path.join(outdir, "cand36.json")
        try:
            subprocess.run([P003R, "tabu", seedf, str(secs),
                            str(rng.randrange(10 ** 9)), outf, str(N)],
                           capture_output=True, timeout=secs + 30)
        except subprocess.TimeoutExpired:
            pass
        stats["trials"] += 1
        try:
            cand = json.load(open(outf))
        except Exception:
            continue
        raw = cand["adjacency"] if isinstance(cand, dict) else cand
        if not raw or len(raw) != N:
            stats["invalid"] += 1
            continue
        A = [[int(c) for c in row] for row in raw]
        k4, i6 = counts(A)
        stats["valid"] += 1
        if k4 or i6:
            stats["invalid"] += 1
            continue
        stats["hits"] += 1
        json.dump({"adjacency": ["".join(str(c) for c in row) for row in A]},
                  open(os.path.join(outdir, "FOUND36_%03d.json" % stats["hits"]), "w"))
        print("  *** 36-VERTEX (4,6)-GRAPH FOUND (trial %d) ***" % t, flush=True)
        break
    stats["seconds"] = round(time.time() - t0, 1)
    json.dump(stats, open(os.path.join(outdir, "HUNT36_STATS.json"), "w"), indent=1)
    print("SUMMARY " + json.dumps(stats), flush=True)


if __name__ == "__main__":
    main()
