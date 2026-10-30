#!/usr/bin/env python3
"""Search cap-internal affine bases with few weight-two exterior points."""
from pathlib import Path
import argparse
import numpy as np


def inverse_mod3(a):
    a = np.concatenate((a.copy() % 3, np.eye(8, dtype=np.int64)), axis=1)
    for col in range(8):
        pivot = next((r for r in range(col, 8) if a[r, col] % 3), None)
        if pivot is None:
            return None
        a[[col, pivot]] = a[[pivot, col]]
        if a[col, col] == 2:
            a[col] = (2 * a[col]) % 3
        for r in range(8):
            if r != col and a[r, col] % 3:
                a[r] = (a[r] - a[r, col] * a[col]) % 3
    return a[:, 8:]


def encode_rows(y):
    powers = (3 ** np.arange(7, -1, -1)).astype(np.int64)
    return y @ powers


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cap')
    ap.add_argument('output_prefix')
    ap.add_argument('--trials', type=int, default=10000)
    ap.add_argument('--seed', type=int, default=1)
    args = ap.parse_args()
    ids = np.array(sorted(int(x) for x in Path(args.cap).read_text().split()), dtype=np.int64)
    powers = (3 ** np.arange(7, -1, -1)).astype(np.int64)
    points = (ids[:, None] // powers) % 3
    rng = np.random.default_rng(args.seed)
    best = 1 << 30
    for trial in range(args.trials):
        chosen = rng.choice(len(ids), size=9, replace=False)
        p0 = points[chosen[0]]
        basis_points = points[chosen[1:]]
        b = (basis_points - p0) % 3
        inv_b = inverse_mod3(b)
        if inv_b is None:
            continue
        d = (points - p0) % 3
        y = (d @ inv_b) % 3
        # Directly verify the basis normalization before considering this trial.
        assert np.array_equal((b @ inv_b) % 3, np.eye(8, dtype=np.int64))
        weights = np.count_nonzero(y, axis=1)
        count2 = int(np.count_nonzero(weights == 2))
        if count2 < best:
            best = count2
            out_ids = np.sort(encode_rows(y))
            out_path = f'{args.output_prefix}-w2-{best}.txt'
            Path(out_path).write_text(''.join(f'{x}\n' for x in out_ids))
            Path(f'{args.output_prefix}-basis.txt').write_text(
                '# source cap ids: origin followed by eight independent differences\n' +
                ''.join(f'{ids[i]}\n' for i in chosen))
            print(f'NEW_BEST trial={trial} weight2={best} output={out_path}', flush=True)
            if best == 0:
                break
        if (trial + 1) % 1000 == 0:
            print(f'PROGRESS trials={trial + 1} best_weight2={best}', flush=True)

if __name__ == '__main__':
    main()
