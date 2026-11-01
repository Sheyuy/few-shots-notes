"""Turn one class of an `ASYM_CHECKPOINT` into an explicit 134,753-point set.

The checkpoint only stores which points were removed from / added to the
auxiliary set `XH`. This script rebuilds that class, solves it (maximising the
number of kept slots), expands the assignment back to points and runs the
official verifier. Output is one 10-digit base-7 string per line, the same
format as `data/scnet/p004/*.txt`.

    export FEW_SHOTS_REPO=/path/to/few-shots
    python3 src/scnet/p004/emit_class.py \
        --checkpoint data/scnet/p004/asym_r1_checkpoint.json --index 18 \
        --rep-old 6,0,6,4,5 --rep-new 6,1,6,5,5 --out /tmp/r1c19.txt

Needs OR-Tools. `--index` counts from 0 in `history`; index 18 is the class the
search reported as class 19 (hash d2172281...)."""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import model as m
import sweep
from ortools.sat.python import cp_model


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--checkpoint', required=True)
    ap.add_argument('--index', type=int, required=True)
    ap.add_argument('--rep-old', required=True, help='e.g. 6,0,6,4,5')
    ap.add_argument('--rep-new', required=True, help='e.g. 6,1,6,5,5')
    ap.add_argument('--out', required=True)
    ap.add_argument('--seconds', type=float, default=60)
    a = ap.parse_args()

    old = tuple(int(x) for x in a.rep_old.split(','))
    new = tuple(int(x) for x in a.rep_new.split(','))
    item = json.load(open(a.checkpoint))['history'][a.index]

    R = [new if x == old else x for x in m.Rlist]
    pairs = sweep.private_pairs(R)
    if len(pairs) != 9:
        raise SystemExit('this R variant has %d private pairs, expected 9' % len(pairs))
    X = [m.tr(x) for x in R]
    X = [(1, 5, 6, 3, 5) if x == (2, 4, 6, 3, 5) else x for x in X]
    fixed = [m.enc5(x) for x in X]
    rootmask = sum(1 << m.enc5(x) for x in X)
    M = ((rootmask & ~sum(1 << x for x in item['state']['remove']))
         | sum(1 << x for x in item['state']['add']))
    p = m.AsymmetricStripModel(pairs, list(m.bits(M)), fixed, baseR=R, compress=True)

    cp = cp_model.CpModel()
    ch = [cp.NewBoolVar('c%d' % i) for i in range(2 * p.nvars)]
    for i in range(p.nvars):
        cp.Add(ch[2 * i] + ch[2 * i + 1] <= 1)
    for u, x, v, y in p.ts.forbidden:
        cp.AddBoolOr([ch[2 * u + x].Not(), ch[2 * v + y].Not()])
    cp.Maximize(sum(p.weights[i] * (ch[2 * i] + ch[2 * i + 1]) for i in range(p.nvars)))
    solver = cp_model.CpSolver()
    solver.parameters.num_search_workers = 4
    solver.parameters.max_time_in_seconds = a.seconds
    st = solver.Solve(cp)
    if st not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        raise SystemExit('no solution: %s' % solver.StatusName(st))

    active = [0 if solver.Value(ch[2 * i]) else (1 if solver.Value(ch[2 * i + 1]) else None)
              for i in range(p.nvars)]
    C = p.points(p.expand(active))
    pts = [''.join(map(str, m.DIGITS[x // m.HALF] + m.DIGITS[x % m.HALF])) for x in sorted(C)]
    with open(a.out, 'w') as f:
        f.write('\n'.join(pts) + '\n')
    verdict = sweep.verifier.verify_bytes(json.dumps({'points': pts}).encode(),
                                          sweep.verifier.load_config())
    print('CLASS', a.index, 'holes', p.active_slots - round(solver.ObjectiveValue()),
          'size', len(pts), 'verdict', verdict)


if __name__ == '__main__':
    main()
