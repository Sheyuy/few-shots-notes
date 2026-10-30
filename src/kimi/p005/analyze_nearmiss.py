#!/usr/bin/env python3
"""Measure a near-miss and compute an exact minimum conflict-line cover.

The number of conflict lines is not the number of points that must be
removed.  This script solves the latter as a minimum vertex-cover MILP.
With --baseline, --keep-new forces every point outside that baseline to stay,
so the cover measures the old-point cost of preserving all new points.
"""
import argparse
import numpy as np
from scipy.optimize import milp, Bounds, LinearConstraint
from scipy.sparse import coo_array
from caplib import (load, save, check_points, conflict_lines, valid_cap,
                    directly_addable)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('points')
    ap.add_argument('--baseline')
    ap.add_argument('--keep-new', action='store_true')
    ap.add_argument('--time', type=float, default=120)
    ap.add_argument('--output')
    args = ap.parse_args()
    S = load(args.points)
    lines = conflict_lines(S)
    print('input', args.points, 'n', len(S), 'unique_range_ok', check_points(S),
          'conflict_lines', len(lines), 'valid_cap', not lines)
    if not lines:
        return
    pos = {x: i for i, x in enumerate(S)}
    rows, cols = [], []
    for r, line in enumerate(lines):
        for x in line:
            rows.append(r)
            cols.append(pos[x])
    A = coo_array((np.ones(len(rows)), (rows, cols)),
                  shape=(len(lines), len(S))).tocsc()
    lb = np.zeros(len(S))
    ub = np.ones(len(S))
    if args.keep_new:
        if not args.baseline:
            raise SystemExit('--keep-new requires --baseline')
        B = set(load(args.baseline))
        for x in S:
            if x not in B:
                ub[pos[x]] = 0
    res = milp(c=np.ones(len(S)), integrality=np.ones(len(S)),
               bounds=Bounds(lb, ub),
               constraints=LinearConstraint(A, np.ones(len(lines)),
                                            np.full(len(lines), np.inf)),
               options={'time_limit': args.time, 'mip_rel_gap': 0})
    print('COVER status', res.status, 'message', res.message,
          'objective', res.fun, 'dual_bound', res.mip_dual_bound,
          'seconds', res.execution_time if hasattr(res, 'execution_time') else 'n/a')
    if res.x is None:
        return
    removed = {S[i] for i, v in enumerate(res.x) if v > .5}
    C = [x for x in S if x not in removed]
    addable = directly_addable(C)
    print('REPAIRED n', len(C), 'removed', len(removed),
          'valid_cap', valid_cap(C), 'directly_addable', len(addable),
          'kept_new', (len(set(C)-set(load(args.baseline)))
                       if args.baseline else 'n/a'))
    print('REMOVED', ' '.join(map(str, sorted(removed))))
    if args.output:
        save(args.output, C, f'size {len(C)} exact_cover {len(removed)}')


if __name__ == '__main__':
    main()
