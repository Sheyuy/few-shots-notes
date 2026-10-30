#!/usr/bin/env python3
"""Add one unit clause per original F_3^8 point variable to a DIMACS model."""
from pathlib import Path
import sys

N = 6561

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
    cap = {int(line) for line in Path(path).read_text().splitlines()
           if line.strip() and not line.startswith('#')}
    assert all(0 <= x < N for x in cap)
    ordered = sorted(cap)
    for i, a in enumerate(ordered):
        for b in ordered[i + 1:]:
            assert third(a, b) not in cap, (a, b, third(a, b))
    return cap

def main():
    if len(sys.argv) != 4:
        raise SystemExit('usage: add_sat_witness.py input.cnf output.cnf cap.txt')
    src, dst, cap_path = sys.argv[1:]
    cap = read_cap(cap_path)
    with open(src) as fi, open(dst, 'w') as fo:
        header = fi.readline().split()
        assert header[:2] == ['p', 'cnf']
        nvars, nclauses = map(int, header[2:4])
        assert nvars >= N
        fo.write(f'p cnf {nvars} {nclauses + N}\n')
        for line in fi:
            fo.write(line)
        for x in range(N):
            fo.write(f'{x + 1 if x in cap else -(x + 1)} 0\n')
    print(f'WROTE {dst} points={len(cap)} added_units={N} clauses={nclauses + N}')

if __name__ == '__main__':
    main()
