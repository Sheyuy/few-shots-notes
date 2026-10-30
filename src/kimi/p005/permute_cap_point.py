#!/usr/bin/env python3
"""Coordinate-permute a cap so a matching source point becomes a target point."""
from pathlib import Path
import argparse

POWERS = [3 ** i for i in range(7, -1, -1)]

def decode(x):
    return [(x // p) % 3 for p in POWERS]

def encode(d):
    return sum(v * p for v, p in zip(d, POWERS))

def signature(d):
    return tuple(d.count(v) for v in (0, 1, 2))

def third(a, b):
    out = 0
    place = 1
    for _ in range(8):
        out += (-(a % 3 + b % 3) % 3) * place
        a //= 3
        b //= 3
        place *= 3
    return out

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cap')
    ap.add_argument('target', type=int)
    ap.add_argument('output')
    ap.add_argument('--source', type=int)
    args = ap.parse_args()
    cap = sorted(int(x) for x in Path(args.cap).read_text().split())
    target_d = decode(args.target)
    source = args.source
    if source is None:
        source = next(x for x in cap if signature(decode(x)) == signature(target_d))
    source_d = decode(source)
    assert signature(source_d) == signature(target_d)
    perm = [-1] * 8
    for value in (0, 1, 2):
        targets = [i for i, v in enumerate(target_d) if v == value]
        sources = [i for i, v in enumerate(source_d) if v == value]
        for i, j in zip(targets, sources):
            perm[i] = j
    assert sorted(perm) == list(range(8))
    transformed = sorted(encode([decode(x)[perm[i]] for i in range(8)]) for x in cap)
    assert len(transformed) == len(cap) and args.target in transformed
    for i, a in enumerate(transformed):
        for b in transformed[i + 1:]:
            assert third(a, b) not in transformed
    Path(args.output).write_text(''.join(f'{x}\n' for x in transformed))
    print(f'PERMUTED source={source} target={args.target} perm={perm} size={len(cap)}')

if __name__ == '__main__':
    main()
