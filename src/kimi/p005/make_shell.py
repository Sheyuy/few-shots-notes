#!/usr/bin/env python3
"""Build the exact bounded-deletion pool around a legal seed cap.

If an improved cap is obtained by deleting at most d seed points, every
newly added outside point must have conflict degree at most d.  Thus it is
enough to solve exactly on seed U {x: degree(x) <= d}.
"""
import argparse
from collections import Counter
from caplib import load, save, valid_cap, outside_conflicts, conflict_lines


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('seed')
    ap.add_argument('remove', type=int, help='maximum deletion radius d')
    ap.add_argument('output')
    args = ap.parse_args()
    seed = sorted(load(args.seed))
    if not valid_cap(seed):
        raise SystemExit('seed is not a legal cap')
    edges = outside_conflicts(seed)
    hist = Counter(len(e) for x, e in enumerate(edges) if e)
    outside = [x for x, e in enumerate(edges) if 0 < len(e) <= args.remove]
    pool = sorted(set(seed) | set(outside))
    save(args.output, pool,
         f'seed_size {len(seed)} remove {args.remove} outside {len(outside)}')
    pos = set(pool)
    nlines = sum(1 for a, b, c in conflict_lines(pool)
                 if a in pos and b in pos and c in pos)
    print('seed', len(seed), 'remove', args.remove,
          'outside', len(outside), 'pool', len(pool), 'lines', nlines)
    print('degree_histogram', ' '.join(f'{k}:{hist[k]}' for k in sorted(hist)))


if __name__ == '__main__':
    main()
