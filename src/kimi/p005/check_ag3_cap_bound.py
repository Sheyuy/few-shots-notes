#!/usr/bin/env python3
"""Independently check max cap size in AG(3,3) with two tiny SAT instances."""
import argparse
import itertools
import subprocess
from pathlib import Path

N = 27
PN = 32
TRUE = 10**9
FALSE = 0
POW3 = (9, 3, 1)


def third(a, b):
    return sum((-((a // p + b // p) % 3) % 3) * p for p in POW3)


class CNF:
    def __init__(self):
        self.clauses = []
        self.nextvar = N + 1

    def emit(self, *lits):
        self.clauses.append(tuple(lits))

    def comparator(self, a, b):
        if a == TRUE or b == TRUE:
            return TRUE, b if a == TRUE else a
        if a == FALSE:
            return b, FALSE
        if b == FALSE:
            return a, FALSE
        if a == b:
            return a, a
        mx, mn = self.nextvar, self.nextvar + 1
        self.nextvar += 2
        self.emit(-a, mx)
        self.emit(-b, mx)
        self.emit(a, b, -mx)
        self.emit(-mn, a)
        self.emit(-mn, b)
        self.emit(mn, -a, -b)
        return mx, mn


def make_model(target):
    cnf = CNF()
    for a in range(N):
        for b in range(a + 1, N):
            c = third(a, b)
            if c > b:
                cnf.emit(-(a + 1), -(b + 1), -(c + 1))
    wires = list(range(1, N + 1)) + [FALSE] * (PN - N)
    p = 1
    while p < PN:
        k = p
        while k >= 1:
            for j in range(k % p, PN - k, 2 * k):
                for i in range(k):
                    if (i + j) // (2 * p) == (i + j + k) // (2 * p):
                        wires[i + j], wires[i + j + k] = cnf.comparator(
                            wires[i + j], wires[i + j + k]
                        )
            k //= 2
        p *= 2
    cnf.emit(wires[target - 1])
    return cnf


def write_cnf(path, cnf):
    with path.open('w') as f:
        f.write(f'p cnf {cnf.nextvar - 1} {len(cnf.clauses)}\n')
        for clause in cnf.clauses:
            f.write(' '.join(map(str, clause)) + ' 0\n')


def check_cap(points):
    points = set(points)
    for a, b in itertools.combinations(sorted(points), 2):
        assert third(a, b) not in points, (a, b, third(a, b))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('kissat')
    ap.add_argument('output_prefix')
    ap.add_argument('--loader')
    args = ap.parse_args()
    prefix = Path(args.output_prefix)
    prefix.parent.mkdir(parents=True, exist_ok=True)
    base_cmd = [args.kissat]
    if args.loader:
        base_cmd.insert(0, args.loader)

    expected = {10: 20, 9: 10}
    for target in (10, 9):
        cnf = make_model(target)
        stem = f'{prefix}-ag3-cap{target}'
        cnf_path = Path(f'{stem}.cnf')
        log_path = Path(f'{stem}-kissat.log')
        write_cnf(cnf_path, cnf)
        with log_path.open('w') as log:
            proc = subprocess.run(
                base_cmd + ['--time=60', '--seed=0', '--sat', str(cnf_path)],
                stdout=log,
                stderr=subprocess.STDOUT,
                check=False,
            )
        if proc.returncode != expected[target]:
            raise RuntimeError(f'target {target}: unexpected Kissat rc {proc.returncode}; see {log_path}')
        text = log_path.read_text()
        if target == 10:
            assert 's UNSATISFIABLE' in text
            print(f'UNSAT AG(3,3) cap size 10; clauses={len(cnf.clauses)} log={log_path}')
        else:
            assert 's SATISFIABLE' in text
            values = []
            for line in text.splitlines():
                if line.startswith('v '):
                    values.extend(int(x) for x in line[2:].split())
            points = [v - 1 for v in values if 1 <= v <= N]
            points = sorted(set(points))
            assert len(points) == 9, points
            check_cap(points)
            out = Path(f'{stem}-points.txt')
            out.write_text(''.join(f'{x}\n' for x in points))
            print(f'SAT AG(3,3) cap size 9; points={points} output={out}')


if __name__ == '__main__':
    main()
