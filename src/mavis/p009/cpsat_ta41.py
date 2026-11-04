#!/usr/bin/env python3
"""cpsat_ta41.py -- CP-SAT model for the ta41 job-shop instance.

Why CP-SAT: our tabu + path-relinking search has been stuck on makespan 2006
for 80+ process-hours.  OR-Tools CP-SAT has a purpose-built job-shop
scheduling engine (interval variables + no-overlap + precedence + a dedicated
search) that is a completely different algorithm from anything we have tried.

Model (the standard job-shop formulation):
  * one interval per operation, size = its processing time
  * within a job: op k+1 starts no earlier than op k ends   (precedence)
  * within a machine: its operations do not overlap          (no-overlap)
  * minimise the makespan = max end

Certificate: {"machines": [[job ids on machine 0 in order], ...]} -- exactly the
format the official verifier expects.

Usage:
  python3 cpsat_ta41.py --instance src/instance.json --time 600 --workers 2 \
                         --seed 1 --out best.json
"""
import argparse
import json
import sys
import time

from ortools.sat.python import cp_model


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--instance", default="src/instance.json")
    ap.add_argument("--time", type=float, default=600.0, help="wall limit in seconds")
    ap.add_argument("--workers", type=int, default=2)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--out", default="cpsat_best.json")
    ap.add_argument("--log", default=None)
    ap.add_argument("--linlevel", type=int, default=0, help="CP-SAT linearization_level")
    ap.add_argument("--disjunctive", type=int, default=0, help="CP-SAT use_disjunctive_constraint")
    ap.add_argument("--cmax-cap", type=int, default=0,
                    help="upper bound on the makespan (0 = no cap).  Capping it at the "
                         "known best (2006) shrinks the search space enormously and "
                         "turns the question into 'is anything <= cap-1 feasible?'")
    args = ap.parse_args()

    inst = json.load(open(args.instance))
    routes = inst["routes"]        # routes[j][k] = machine of job j's k-th op
    durations = inst["durations"]   # durations[j][k] = processing time of that op
    nj = len(routes)
    nm = len(routes[0])
    nops = sum(len(r) for r in routes)

    model = cp_model.CpModel()
    starts, ends = {}, {}
    horizon = 0
    for j in range(nj):
        horizon += sum(durations[j])
    horizon = max(horizon, 1)

    for j in range(nj):
        prev_end = None
        for k, m in enumerate(routes[j]):
            d = durations[j][k]
            key = (j, k)
            s = model.NewIntVar(0, horizon, f"s_{j}_{k}")
            e = model.NewIntVar(0, horizon, f"e_{j}_{k}")
            model.Add(e == s + d)
            starts[key] = s
            ends[key] = e
            if prev_end is not None:
                model.Add(s >= prev_end)
            prev_end = e

    by_machine = {}
    for j in range(nj):
        for k, m in enumerate(routes[j]):
            by_machine.setdefault(m, []).append((j, k))

    for m, ops in by_machine.items():
        model.AddNoOverlap([model.NewIntervalVar(starts[o], durations[o[0]][o[1]], ends[o],
                                                f"i_{o[0]}_{o[1]}") for o in ops])

    cmax = model.NewIntVar(0, horizon, "cmax")
    for key, e in ends.items():
        model.Add(cmax >= e)
    if args.cmax_cap > 0:
        # cap the horizon as well: a tight domain makes the propagation strong
        model.Add(cmax <= args.cmax_cap)
        for key, s_ in starts.items():
            model.Add(s_ <= args.cmax_cap)
    model.Minimize(cmax)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = args.time
    solver.parameters.num_search_workers = args.workers
    solver.parameters.random_seed = args.seed
    solver.parameters.linearization_level = args.linlevel
    # not every ortools version has this knob -- set it only if it exists
    if hasattr(solver.parameters, "use_disjunctive_constraint"):
        solver.parameters.use_disjunctive_constraint = bool(args.disjunctive)
    solver.parameters.log_search_progress = bool(args.log)
    if args.log:
        solver.log_callback = lambda s: print(s, flush=True)
    t0 = time.time()
    status = solver.Solve(model)
    el = time.time() - t0
    name = solver.StatusName(status)
    print(f"status={name} makespan={solver.ObjectiveValue() if status in (cp_model.OPTIMAL, cp_model.FEASIBLE) else 'n/a'} "
          f"bound={solver.BestObjectiveBound() if status in (cp_model.OPTIMAL, cp_model.FEASIBLE) else 'n/a'} "
          f"elapsed={el:.1f}s", flush=True)
    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return 1
    machines = [[] for _ in range(nm)]
    for m, ops in by_machine.items():
        ops_sorted = sorted(ops, key=lambda o: solver.Value(starts[o]))
        machines[m] = [j for (j, k) in ops_sorted]
    json.dump({"machines": machines}, open(args.out, "w"))
    print("certificate written to", args.out, flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
