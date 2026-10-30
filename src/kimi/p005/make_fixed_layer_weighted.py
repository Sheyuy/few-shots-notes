#!/usr/bin/env python3
"""Build protected, banned, and low-conflict initial sets for fixed-layer search."""
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


def read_cap(path):
    points = set()
    for line in Path(path).read_text().splitlines():
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        x = int(line)
        assert 0 <= x < N and x not in points
        points.add(x)
    ordered = sorted(points)
    for i, a in enumerate(ordered):
        for b in ordered[i + 1:]:
            assert third(a, b) not in points, (a, b, third(a, b))
    return points


def write_points(path, points):
    Path(path).write_text(''.join(f'{x}\n' for x in sorted(points)))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('baseline')
    parser.add_argument('coordinate', type=int)
    parser.add_argument('value', type=int)
    parser.add_argument('prefix')
    args = parser.parse_args()
    assert 0 <= args.coordinate < 8 and 0 <= args.value < 3
    cap = read_cap(args.baseline)
    coordinate = lambda x: (x // POW3[args.coordinate]) % 3
    fixed = {x for x in cap if coordinate(x) == args.value}
    assert fixed
    banned = {x for x in range(N) if coordinate(x) == args.value and x not in cap}

    degree = [0] * N
    ordered = sorted(cap)
    for i, a in enumerate(ordered):
        for b in ordered[i + 1:]:
            degree[third(a, b)] += 1
    best_degree, best = min((degree[x], x) for x in range(N)
                            if x not in cap and coordinate(x) != args.value)
    initial = sorted(cap | {best})
    bad = 0
    for i, a in enumerate(initial):
        for b in initial[i + 1:]:
            c = third(a, b)
            if c > b and c in initial:
                bad += 1
    assert bad == best_degree

    base_path = f'{args.prefix}-base.txt'
    banned_path = f'{args.prefix}-banned.txt'
    initial_path = f'{args.prefix}-initial{len(initial)}.txt'
    write_points(base_path, fixed)
    write_points(banned_path, banned)
    write_points(initial_path, initial)
    print(f'FIXED {len(fixed)} BANNED {len(banned)} ADDED {best} '
          f'DEGREE {best_degree} INITIAL {len(initial)} BAD {bad}')
    print(f'WROTE {base_path} {banned_path} {initial_path}')


if __name__ == '__main__':
    main()
