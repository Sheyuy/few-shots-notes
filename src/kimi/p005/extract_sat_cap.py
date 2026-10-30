#!/usr/bin/env python3
"""Extract and independently verify original point variables from a Kissat log."""
from pathlib import Path
import argparse

N = 6561
BASIS = (0, 2187, 729, 243, 81, 27, 9, 3, 1)

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
    ap.add_argument('log')
    ap.add_argument('output')
    ap.add_argument('--extra', type=int, action='append', default=[])
    ap.add_argument('--min-size', type=int, default=513)
    args = ap.parse_args()
    sat = False
    cap = set()
    for line in Path(args.log).read_text(errors='replace').splitlines():
        if line.startswith('s '):
            sat = line.strip() == 's SATISFIABLE'
        elif line.startswith('v '):
            for token in line[2:].split():
                lit = int(token)
                if 0 < lit <= N:
                    cap.add(lit - 1)
    if not sat:
        raise SystemExit('log does not contain s SATISFIABLE')
    cap.update(BASIS)
    cap.update(args.extra)
    assert len(cap) >= args.min_size, len(cap)
    assert all(0 <= x < N for x in cap)
    ordered = sorted(cap)
    for i, a in enumerate(ordered):
        for b in ordered[i + 1:]:
            c = third(a, b)
            assert c not in cap, (a, b, c)
    Path(args.output).write_text(''.join(f'{x}\n' for x in ordered))
    print(f'VERIFIED size={len(cap)} output={args.output}')

if __name__ == '__main__':
    main()
