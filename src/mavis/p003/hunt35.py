#!/usr/bin/env python3
"""
hunt35.py -- hunt for new 35-vertex (4,6)-counterexamples.

Why
---
Lehavi's Algorithm 1: if a 36-vertex (4,6)-graph exists, then ALL of its
36 vertex-deleted subgraphs are (4,6)-graphs.  So the only remaining way to a
world record is to know more 35-vertex counterexamples than we currently do
(we have 134; the published set has 37).

Our closure over those 134 is already at a fixed point, so the remaining lever
is a fresh search.

Mechanism
---------
  base graph  --flip k random edges-->  (usually invalid)
               --p003r tabu-->           a valid (4,6)-graph
The tabu converges back to *some* valid graph; if it lands in a basin we have
not seen, that is a new isomorphism class.

Safety
------
The search's own violation counter is NOT trusted.  Every candidate goes
through validate35.py's independent brute-force check (count K4s, enumerate
independent 5-sets and look for a vertex adjacent to none) and only then
through a 4-round WL dedup against the known base set.

Usage:
  python3 hunt35.py <pool.json> <outdir> <n_trials> <secs_per_run> [seed]
"""
import json
import os
import random
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from validate35 import counts, wl_hash   # noqa: E402

P003R = "./p003r"


def masks_from_rows(rows):
    n = len(rows)
    masks = [0] * n
    for i in range(n):
        r = rows[i]
        for j in range(n):
            if i != j and r[j] == "1":
                masks[i] |= 1 << j
    return masks


def rows_from_masks(masks):
    n = len(masks)
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
    known = set(wl_hash([[int(c) for c in row] for row in rows]) for rows in pool)
    print("base pool: %d graphs, %d distinct WL classes" % (len(pool), len(known)),
          flush=True)

    rng = random.Random(seed0)
    seen = set(known)
    stats = {"trials": 0, "valid": 0, "invalid": 0, "novel": 0}
    t0 = time.time()
    khist = {}
    for t in range(trials):
        bi = rng.randrange(len(pool))
        k = rng.choice([4, 8, 12, 16, 24, 32, 48, 64, 80])
        khist[k] = khist.get(k, 0) + 1
        masks = perturb(masks_from_rows(pool[bi]), rng, k)
        rows = rows_from_masks(masks)
        seedf = os.path.join(outdir, "seed.json")
        json.dump({"adjacency": rows}, open(seedf, "w"))
        outf = os.path.join(outdir, "cand.json")
        try:
            subprocess.run([P003R, "tabu", seedf, str(secs),
                            str(rng.randrange(10 ** 9)), outf, "35"],
                           capture_output=True, timeout=secs + 30)
        except subprocess.TimeoutExpired:
            pass
        stats["trials"] += 1
        try:
            cand = json.load(open(outf))
        except Exception:
            continue
        raw = cand["adjacency"] if isinstance(cand, dict) else cand
        if not raw or len(raw) != 35:
            stats["invalid"] += 1
            continue
        A = [[int(c) for c in row] for row in raw]
        k4, i6 = counts(A)
        if k4 or i6:
            stats["invalid"] += 1
            continue
        stats["valid"] += 1
        h = wl_hash(A)
        if h in seen:
            continue
        seen.add(h)
        stats["novel"] += 1
        fn = os.path.join(outdir, "NEW35_%03d.json" % stats["novel"])
        json.dump({"adjacency": ["".join(str(c) for c in row) for row in A]},
                  open(fn, "w"))
        print("  NEW isomorphism class #%d (trial %d, base %d, k=%d)"
              % (stats["novel"], t, bi, k), flush=True)
        if stats["novel"] % 10 == 0:
            json.dump(stats, open(os.path.join(outdir, "HUNT_STATS.json"), "w"))
    stats["distinct_classes_total"] = len(seen)
    stats["seconds"] = round(time.time() - t0, 1)
    stats["khist"] = dict(sorted(khist.items()))
    json.dump(stats, open(os.path.join(outdir, "HUNT_STATS.json"), "w"), indent=1)
    print("SUMMARY " + json.dumps(stats), flush=True)


if __name__ == "__main__":
    main()
