#!/usr/bin/env python3
"""Recover and independently verify a cap from a fixed-layer SAT log."""
import argparse
from pathlib import Path

N = 6561
POW3 = (2187, 729, 243, 81, 27, 9, 3, 1)


def third(a, b):
    out = 0
    place = 1
    for _ in range(8):
        out += (-(a % 3 + b % 3) % 3) * place
        a //= 3
        b //= 3
        place *= 3
    return out


def read_points(path):
    points = {int(token) for token in Path(path).read_text().split()
              if token and not token.startswith('#')}
    assert all(0 <= x < N for x in points)
    return points


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('log')
    parser.add_argument('mapping')
    parser.add_argument('fixed_source')
    parser.add_argument('coordinate', type=int)
    parser.add_argument('value', type=int)
    parser.add_argument('output')
    parser.add_argument('--min-size', type=int, default=513)
    args = parser.parse_args()
    assert 0 <= args.coordinate < 8 and 0 <= args.value < 3

    point_of_var = {}
    for line in Path(args.mapping).read_text().splitlines():
        if not line.strip() or line.startswith('#'):
            continue
        var, point = map(int, line.split())
        assert var not in point_of_var
        point_of_var[var] = point
    source = read_points(args.fixed_source)
    fixed = {x for x in source
             if (x // POW3[args.coordinate]) % 3 == args.value}
    assert fixed

    sat = False
    cap = set(fixed)
    for line in Path(args.log).read_text(errors='replace').splitlines():
        if line.startswith('s '):
            sat = line.strip() == 's SATISFIABLE'
        elif line.startswith('v '):
            for token in line[2:].split():
                lit = int(token)
                if lit > 0 and lit in point_of_var:
                    cap.add(point_of_var[lit])
    if not sat:
        raise SystemExit('log does not contain s SATISFIABLE')
    assert len(cap) >= args.min_size, len(cap)
    ordered = sorted(cap)
    assert len(ordered) == len(cap)
    for i, a in enumerate(ordered):
        for b in ordered[i + 1:]:
            c = third(a, b)
            assert c not in cap, (a, b, c)
    Path(args.output).write_text(''.join(f'{x}\n' for x in ordered))
    print(f'VERIFIED size={len(cap)} fixed={len(fixed)} output={args.output}')


if __name__ == '__main__':
    main()
