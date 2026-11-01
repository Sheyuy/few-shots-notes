#!/usr/bin/env python3
"""card.py -- WORK IN PROGRESS / NEGATIVE RESULT: a *verified* "at most k of m literals" CNF encoding.

History (why this file exists): two earlier attempts at a Sinz-style sequential
counter were WRONG.  The first was silently over-constraining (it created
counter variables s[i][j] with j > i, which are always false, but only forced
them false conditionally), so a known-good pinned graph came back UNSAT.  The
second got the polarities of the "down" clauses backwards.

The encoding below is derived from scratch and then VERIFIED EXHAUSTIVELY:
for m <= 7 and every k, every assignment of the m literals is checked with
kissat against a brute-force reference.  Run `python3 card.py` for the test.

Encoding
---------
s[i][j] is meant to be true iff "at least j of x_1..x_i are true",
with s[i][j] only created for 1 <= j <= min(k+1, i).
s[0][0] is the constant TRUE, s[0][j>0] and s[i][j>i] are the constant FALSE.

For i = 2..m, j = 1..min(k+1,i):
    U1:  (-x_i | -s[i-1][j-1] | s[i][j])      up:   (x_i & >=j-1 before)  ->  >=j
    U2:  ( x_i | -s[i-1][j]   | s[i][j])      up:   (~x_i & >=j before)  ->  >=j
    D1:  (-s[i][j] |  x_i  | s[i-1][j])       down: s[i][j] & x_i       ->  >=j-1 before
    D2:  (-s[i][j] | s[i-1][j-1] | -x_i)      down: s[i][j] & ~x_i      ->  >=j-1 before
    D3:  (-s[i][j] | s[i-1][j-1] | s[i-1][j]) down: s[i][j]            ->  >=j-1 before
Base i = 1:  (-x_1 | s[1][1])  and  (-s[1][1] | x_1)
Final:        (-s[m][k+1])
"""
import itertools
import os
import subprocess
import sys


def at_most_k(lits, k, next_var):
    """Return (clauses, next_var).  `lits` are signed DIMACS literals."""
    m = len(lits)
    if k >= m:
        return [], next_var
    if k < 0:
        return [[-l] for l in lits], next_var

    s = {}
    for i in range(1, m + 1):
        for j in range(1, min(k + 1, i) + 1):
            s[(i, j)] = next_var
            next_var += 1

    def prev(i, j):
        """s[i-1][j] if it exists (1<=j<=i-1), else None (= constant FALSE)."""
        return s.get((i - 1, j))

    T = "TRUE"     # sentinel: the referenced counter is the constant TRUE
    F = "FALSE"    # sentinel: the referenced counter is the constant FALSE

    cl = []
    # base case i = 1:  s[1][1] <-> x_1
    cl.append([-lits[0], s[(1, 1)]])
    cl.append([-s[(1, 1)], lits[0]])

    for i in range(2, m + 1):
        x = lits[i - 1]
        for j in range(1, min(k + 1, i) + 1):
            sij = s[(i, j)]
            # s[i-1][j-1]: constant TRUE when j == 1, otherwise a real variable
            sm1 = T if j == 1 else s[(i - 1, j - 1)]
            # s[i-1][j]: constant FALSE when j > i-1, otherwise a real variable
            sp = F if j > i - 1 else s[(i - 1, j)]

            # U1: (-x | -sm1 | s)      sm1 == TRUE -> -TRUE is false, drop it
            cl.append([-x] + ([] if sm1 is T else [-sm1]) + [sij])
            # U2: ( x | -sp  | s)      sp == FALSE -> -FALSE is true, clause is
            #                            trivially satisfied, skip it
            if sp is not F:
                cl.append([x, -sp, sij])
            # D1: (-s |  x  | sp)      sp == FALSE -> drop it
            cl.append([-sij, x] + ([] if sp is F else [sp]))
            # D2: (-s | sm1 | -x)      sm1 == TRUE -> drop it
            cl.append([-sij] + ([] if sm1 is T else [sm1]) + [-x])
            # D3: (-s | sm1 | sp)
            cl.append([-sij] + ([] if sm1 is T else [sm1]) + ([] if sp is F else [sp]))
    # final
    cl.append([-s[(m, k + 1)]])
    return cl, next_var


def write_cnf(path, clauses, nvars):
    with open(path, "w") as f:
        f.write("p cnf %d %d\n" % (nvars, len(clauses)))
        for c in clauses:
            f.write(" ".join(str(x) for x in c) + " 0\n")


def selftest(solver="./kissat", tmpdir="/tmp"):
    ok = True
    for m in range(1, 8):
        for k in range(0, min(m, 5)):
            lits = list(range(1, m + 1))
            clauses, nv = at_most_k(lits, k, m + 1)
            bad = 0
            for bits in itertools.product([0, 1], repeat=m):
                extra = [[(i + 1)] if b else [-(i + 1)] for i, b in enumerate(bits)]
                cn = clauses + extra
                path = os.path.join(tmpdir, "card_test.cnf")
                write_cnf(path, cn, nv)
                r = subprocess.run([solver, path], capture_output=True, text=True,
                                   timeout=180)
                sat = "SATISFIABLE" in r.stdout and "UNSATISFIABLE" not in r.stdout
                want = sum(bits) <= k
                if sat != want:
                    bad += 1
                    if bad <= 3:
                        print("  FAIL m=%d k=%d bits=%s got=%s want=%s"
                              % (m, k, "".join(map(str, bits)), sat, want))
            print("m=%d k=%d  %s" % (m, k, "ok" if bad == 0 else "BROKEN(%d)" % bad))
            if bad:
                ok = False
    print("SELF-TEST:", "PASS" if ok else "FAIL")
    return ok


if __name__ == "__main__":
    sys.exit(0 if selftest() else 1)
