"""Rebuild the best nine-pair strip construction and minimise omitted slots.

For a fixed nine-pair family the model is a 2-SAT instance; deleting a slot is
allowed, so the largest achievable set is  raw_size - (minimum omissions).
The record target is 134754, i.e. at most 16 omissions for the R2 family.

Usage:
    python3 frontier9.py --variant R2 --x patch --seconds 60 --write 9pair.txt

Requires OR-Tools (only for this optimisation step); the official verifier is
run in-process and is the only judge of the printed score.
"""
import argparse
import importlib.util
import json
from pathlib import Path

import model as m

def _repo_root():
    import os
    for c in [os.environ.get('FEW_SHOTS_REPO'),str(Path.cwd()),'/Users/shiny/.sclaw/workspace/few-shots']:
        if c and (Path(c)/'problems/p004-shannon-capacity-c7'/'verify'/'verify.py').exists():return Path(c)
    raise SystemExit('set FEW_SHOTS_REPO to a few-shots clone containing problems/p004-shannon-capacity-c7')
ROOT=_repo_root()/'problems/p004-shannon-capacity-c7'
_spec = importlib.util.spec_from_file_location('p004_official_verifier', ROOT / 'verify' / 'verify.py')
verifier = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(verifier)

VARIANTS = {
    # name: (replaced R point, replacement)   -- as in the paper's construction
    'R1': ((6, 0, 6, 4, 5), (6, 1, 6, 5, 5)),
    'R2': ((6, 4, 5, 3, 2), (6, 5, 5, 3, 1)),
}


def private_pairs(R):
    Rm = sum(1 << m.enc5(x) for x in R)
    pairs = []
    for q in range(m.HALF):
        if (Rm >> q) & 1:
            continue
        b = m.N[q] & Rm
        if b.bit_count() == 1:
            pairs.append((m.DIGITS[b.bit_length() - 1], m.DIGITS[q]))
    return pairs


def build(variant, xmode):
    old, new = VARIANTS[variant]
    R = [new if x == old else x for x in m.Rlist]
    pairs = private_pairs(R)
    if len(pairs) != 9:
        raise RuntimeError('expected 9 private pairs, got %d' % len(pairs))
    X0 = [m.tr(x) for x in m.Rlist]
    X0[267] = (1, 5, 6, 3, 5)
    raw = [m.tr(x) for x in R]
    patched = [(1, 5, 6, 3, 5) if x == (2, 4, 6, 3, 5) else x for x in raw]
    X = {'x0': X0, 'tr': raw, 'patch': patched}[xmode]
    return m.StripModel(pairs, X, baseR=R, compress=False), R, X


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--variant', choices=sorted(VARIANTS), default='R2')
    ap.add_argument('--x', choices=['x0', 'tr', 'patch'], default='patch')
    ap.add_argument('--seconds', type=float, default=60.0)
    ap.add_argument('--workers', type=int, default=4)
    ap.add_argument('--write')
    a = ap.parse_args()

    from ortools.sat.python import cp_model

    problem, R, X = build(a.variant, a.x)
    cp = cp_model.CpModel()
    choice = [cp.NewBoolVar('c%d' % i) for i in range(2 * problem.nvars)]
    for v in range(problem.nvars):
        cp.Add(choice[2 * v] + choice[2 * v + 1] <= 1)
    for u, x1, v, x2 in problem.ts.forbidden:
        cp.AddBoolOr([choice[2 * u + x1].Not(), choice[2 * v + x2].Not()])
    cp.Minimize(sum(1 - choice[2 * v] - choice[2 * v + 1] for v in range(problem.nvars)))

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = a.seconds
    solver.parameters.num_search_workers = a.workers
    solver.parameters.random_seed = 771001
    status = solver.Solve(cp)
    name = solver.StatusName(status)
    print('FRONTIER9', json.dumps({'variant': a.variant, 'x': a.x, 'status': name,
                                   'active_slots': problem.nvars,
                                   'raw_size': problem.raw_size}), flush=True)
    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return 1
    best = int(round(solver.ObjectiveValue()))
    print('FRONTIER9_BOUND', json.dumps({'omissions': best,
                                         'proven_optimal': status == cp_model.OPTIMAL,
                                         'size': problem.raw_size - best}), flush=True)
    active = [0 if solver.Value(choice[2 * v]) else (1 if solver.Value(choice[2 * v + 1]) else None)
              for v in range(problem.nvars)]
    full = problem.expand(active)
    C = problem.points(full)
    pts = [''.join(map(str, m.DIGITS[x // m.HALF] + m.DIGITS[x % m.HALF])) for x in sorted(C)]
    verdict = verifier.verify_bytes(json.dumps({'points': pts}).encode(), verifier.load_config())
    print('FRONTIER9_VERIFY', json.dumps(verdict), flush=True)
    if a.write:
        Path(a.write).write_text('\n'.join(pts) + '\n')
        print('FRONTIER9_WROTE', a.write, len(pts), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
