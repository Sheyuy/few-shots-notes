#!/usr/bin/env python3
"""pick_best.py -- pick the best ta41 schedule among the candidates produced by
jssp15 and recompute its makespan independently.

The makespan is recomputed from the instance data (routes + durations) with a
longest-path computation on the disjunctive graph, i.e. it does NOT trust the
solver's own bookkeeping.  The official verifier does the same thing later, but
catching a discrepancy here means a broken candidate never leaves the runner.

Usage: python3 pick_best.py <instance.json> <outdir> <best.json>
"""
import glob
import json
import os
import sys


def makespan(machines, routes, durations):
    """Longest path over the precedence graph of a job-shop schedule.

    machines[m] = list of job ids in the order they are processed on machine m
    routes[j]    = machine sequence of job j
    durations[j][m] = processing time of job j on machine m
    """
    nm = len(machines)
    nj = len(routes)
    # index of each op: op id = job * nm + position within the job
    op_machine = {}
    op_dur = {}
    op_job = {}
    op_pred_job = {}      # previous op of the same job
    op_next_job = {}
    prev_on_machine = {}  # previous op on the same machine
    for j in range(nj):
        for k, m in enumerate(routes[j]):
            oid = j * nm + k
            op_machine[oid] = m
            # durations are indexed by the POSITION IN THE ROUTE, not by machine
            op_dur[oid] = durations[j][k]
            op_job[oid] = j
    for m in range(nm):
        prev = None
        for j in machines[m]:
            k = routes[j].index(m)
            oid = j * nm + k
            prev_on_machine[oid] = prev
            prev = oid
    # build job chains
    for j in range(nj):
        prev = None
        for k in range(len(routes[j])):
            oid = j * nm + k
            op_pred_job[oid] = prev
            op_next_job[oid] = None
            if prev is not None:
                op_next_job[prev] = oid
            prev = oid
    # longest path (the graph is acyclic by construction of a feasible schedule)
    indeg = {}
    succ = {}
    for oid in op_dur:
        preds = []
        if op_pred_job[oid] is not None:
            preds.append(op_pred_job[oid])
        if prev_on_machine[oid] is not None:
            preds.append(prev_on_machine[oid])
        indeg[oid] = len(preds)
        for p in preds:
            succ.setdefault(p, []).append(oid)
    est = {}
    stack = [o for o in op_dur if indeg[o] == 0]
    # topological order via Kahn
    order = []
    q = list(stack)
    indeg2 = dict(indeg)
    while q:
        o = q.pop()
        order.append(o)
        for s in succ.get(o, []):
            indeg2[s] -= 1
            if indeg2[s] == 0:
                q.append(s)
    if len(order) != len(op_dur):
        return None            # cycle -> infeasible
    for o in order:
        e = 0
        if op_pred_job[o] is not None:
            e = max(e, est[op_pred_job[o]] + op_dur[op_pred_job[o]])
        if prev_on_machine[o] is not None:
            e = max(e, est[prev_on_machine[o]] + op_dur[prev_on_machine[o]])
        est[o] = e
    return max(est[o] + op_dur[o] for o in op_dur)


def main():
    inst_path = sys.argv[1] if len(sys.argv) > 1 else "src/mavis/p009/instance.json"
    outdir = sys.argv[2] if len(sys.argv) > 2 else "out"
    best_path = sys.argv[3] if len(sys.argv) > 3 else "out/best.json"
    inst = json.load(open(inst_path))
    routes, durations = inst["routes"], inst["durations"]
    best = None
    bestms = None
    for f in sorted(glob.glob(os.path.join(outdir, "s*.json"))):
        try:
            d = json.load(open(f))
        except Exception:
            continue
        if "machines" not in d:
            continue
        ms = makespan(d["machines"], routes, durations)
        print("  candidate %s -> makespan %s" % (f, ms))
        if ms is None:
            continue
        if bestms is None or ms < bestms:
            bestms, best = ms, d
    if best is None:
        print("no valid candidate")
        return 1
    json.dump(best, open(best_path, "w"))
    print("BEST MAKESPAN:", bestms, "->", best_path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
