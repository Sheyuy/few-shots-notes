#!/usr/bin/env python3
"""cpsat_golomb.py -- CP-SAT model for a short Golomb ruler with n marks.

A Golomb ruler with marks 0 = m_0 < m_1 < ... < m_{n-1} = L has every pairwise
difference distinct.  The board problem p014 asks for n = 29 marks with
L <= 622 (the baseline is 623, from Singer's 1938 projective-plane
construction).

Model (tiny, and it fits CP-SAT's dedicated AllDifferent propagator):
  * one integer per mark, x_0 = 0 (shifting does not change the length),
    strictly increasing, x_{n-1} = L
  * one integer per pair, d_ij = x_j - x_i   (i < j)
  * AllDifferent over all the d_ij  <=>  all pairwise differences distinct

With --target 0 the length is minimised instead of fixed, so CP-SAT either
finds a shorter ruler or proves the current one optimal.

Usage:
  python3 cpsat_golomb.py --n 29 --target 622 --time 600 --workers 2 --out marks.json
"""
import argparse
import json
import sys
import time

from ortools.sat.python import cp_model


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--n", type=int, default=29)
    ap.add_argument("--target", type=int, default=622,
                    help="required length; 0 = minimise instead")
    ap.add_argument("--time", type=float, default=600.0)
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--out", default="marks.json")
    args = ap.parse_args()
    n = args.n

    model = cp_model.CpModel()
    hi = args.target if args.target > 0 else 2000
    x = [model.NewIntVar(0, hi, f"x{i}") for i in range(n)]
    model.Add(x[0] == 0)
    for i in range(n - 1):
        model.Add(x[i + 1] >= x[i] + 1)
    if args.target > 0:
        model.Add(x[n - 1] == args.target)
    else:
        model.Minimize(x[n - 1])

    diffs = []
    for i in range(n):
        for j in range(i + 1, n):
            d = model.NewIntVar(1, hi, f"d_{i}_{j}")
            model.Add(d == x[j] - x[i])
            diffs.append(d)
    model.AddAllDifferent(diffs)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = args.time
    solver.parameters.num_search_workers = args.workers
    solver.parameters.random_seed = args.seed
    t0 = time.time()
    status = solver.Solve(model)
    el = time.time() - t0
    name = solver.StatusName(status)
    print(f"status={name} elapsed={el:.1f}s", flush=True)
    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        print("no ruler found")
        return 1
    marks = [solver.Value(x[i]) for i in range(n)]
    ok = check_ruler(marks)
    print("marks:", marks)
    print("length:", marks[-1] - marks[0], "| valid Golomb ruler:", ok)
    if not ok:
        return 2
    json.dump({"marks": marks}, open(args.out, "w"))
    print("certificate written to", args.out, flush=True)
    return 0


def check_ruler(marks):
    """Exactly the check the official verifier does: no repeated difference."""
    if any(marks[i] >= marks[i + 1] for i in range(len(marks) - 1)):
        return False
    seen = {}
    for j in range(len(marks)):
        for i in range(j):
            d = marks[j] - marks[i]
            if d in seen:
                return False
            seen[d] = (i, j)
    return True


if __name__ == "__main__":
    sys.exit(main())
