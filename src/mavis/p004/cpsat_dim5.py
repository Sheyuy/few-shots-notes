#!/usr/bin/env python3
"""cpsat_dim5.py -- CP-SAT model for "is there an independent set of size N in C7^5?".

The graph: vertices are Z_7^5 (16807 of them); x ~ y iff |x_i - y_i|_Lee <= 1 in
EVERY coordinate (the square strong product), so each vertex has a closed
neighbourhood of 3^5 = 243 points.

Model:
  * one boolean per vertex: is it in the set?
  * for every pair of adjacent vertices: not both selected
  * sum(selected) >= N

If CP-SAT finds a solution we have an independent set of size N in C7^5 (and
N^2 points in C7^10 -- a board-record certificate for p004 if N = 368).
If it proves infeasibility, alpha(C7^5) < N, which is a clean theorem.

Usage:
  python3 cpsat_dim5.py --target 368 --time 600 --workers 2 --seed 1 --out set.txt
"""
import argparse
import itertools
import json
import sys
import time

from ortools.sat.python import cp_model

N5 = 16807
CYC = 7
DIM = 5


def digits(v):
    d = []
    for _ in range(DIM):
        d.append(v % CYC)
        v //= CYC
    return d


def check_independent(pts):
    """Verify that a list of 5-digit base-7 points is an independent set in C7^5."""
    idx = []
    for p in pts:
        v = 0
        for ch in p:
            v = v * CYC + int(ch)
        idx.append(v)
    if len(set(idx)) != len(idx):
        return False, "duplicate point"
    for a in range(len(idx)):
        for b in range(a + 1, len(idx)):
            da, db = dig[idx[a]], dig[idx[b]]
            bad = False
            for i in range(DIM):
                d = abs(da[i] - db[i])
                if d == 6:
                    d = 1
                if d > 1:
                    bad = True
                    break
            if not bad:
                return False, f"points {pts[a]} and {pts[b]} are adjacent"
    return True, f"{len(idx)} points, all pairwise non-adjacent"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", type=int, default=368)
    ap.add_argument("--time", type=float, default=600.0)
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--out", default="dim5_set.txt")
    args = ap.parse_args()

    dig = [digits(v) for v in range(N5)]

    def adj(a, b):
        da, db = dig[a], dig[b]
        any_diff = False
        for i in range(DIM):
            d = abs(da[i] - db[i])
            if d == 6:
                d = 1
            if d > 1:
                return False
            if d == 1:
                any_diff = True
        return any_diff

    model = cp_model.CpModel()
    x = [model.NewBoolVar(f"x{v}") for v in range(N5)]

    # conflict constraints: for every vertex, its 242 neighbours (dedup the pair)
    ncon = 0
    for v in range(N5):
        seen = set()
        dv = dig[v]
        for t in itertools.product(range(CYC), repeat=DIM):
            # offsets -1, 0, +1 per coordinate
            d = [(dv[i] + t[i] - 1) % CYC for i in range(DIM)]
            if d == dv:
                continue
            w = 0
            for c in reversed(d):
                w = w * CYC + c
            if w in seen or w == v:
                continue
            seen.add(w)
            if w > v:
                model.Add(x[v] + x[w] <= 1)
                ncon += 1

    model.Add(sum(x) >= args.target)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = args.time
    solver.parameters.num_search_workers = args.workers
    solver.parameters.random_seed = args.seed
    solver.parameters.log_search_progress = False
    t0 = time.time()
    status = solver.Solve(model)
    el = time.time() - t0
    name = solver.StatusName(status)
    line = f"status={name} conflicts={ncon} elapsed={el:.1f}s"
    print(line, flush=True)
    try:
        open(args.out + ".log", "a").write(line + "\n")
    except Exception:
        pass
    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        print("no solution found")
        return 1
    chosen = [v for v in range(N5) if solver.Value(x[v])]
    with open(args.out, "w") as f:
        for v in chosen:
            f.write("".join(str(c) for c in dig[v]) + "\n")
    print(f"independent set of size {len(chosen)} written to {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
