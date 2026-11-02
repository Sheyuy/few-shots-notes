#!/usr/bin/env python3
"""card2.py -- "at most k of m literals" CNF encoding, derived from scratch
and verified EXHAUSTIVELY against a brute-force reference.

Semantics of the counter:
    s[i][j]  <->  "at least j of L_1..L_i are true"
    s[0][0] = TRUE (const),  s[0][j>0] = FALSE (const),  s[i][j>i] = FALSE (const)

Clauses, for 1 <= i <= m, 1 <= j <= min(k+1, i):
    U1  (-L_i | s[i][1])                       up
    U2  (-s[i-1][j] | s[i][j])                 up   (skip if s[i-1][j] is the FALSE const)
    U3  (-L_i | -s[i-1][j-1] | s[i][j])        up   (only j >= 2)
    D1  (-s[i][j] | -L_i | s[i-1][j-1])        down (skip if s[i-1][j-1] is the TRUE const)
    D2  (-s[i][j] |  L_i | s[i-1][j])          down
    base i=1:  (-L_1 | s[1][1])  and  (-s[1][1] | L_1)
    final:     (-s[m][k+1])

The UP clauses force s true when the count justifies it; the DOWN clauses
force s false when it does not.  Together they make s *exact*, which is what
makes the final clause sound.
"""
import itertools
import os
import subprocess
import sys

TRUE = "T"
FALSE = "F"


def at_most_k(lits, k, next_var):
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

    cl = []
    # base i = 1
    cl.append([-lits[0], s[(1, 1)]])
    cl.append([-s[(1, 1)], lits[0]])

    for i in range(2, m + 1):
        x = lits[i - 1]
        for j in range(1, min(k + 1, i) + 1):
            sij = s[(i, j)]
            A = TRUE if j == 1 else s[(i - 1, j - 1)]     # s[i-1][j-1]
            B = FALSE if j > i - 1 else s[(i - 1, j)]    # s[i-1][j]
            # U1
            if j == 1:
                cl.append([-x, sij])
            # U2
            if B is not FALSE:
                cl.append([-B, sij])
            # U3
            if j >= 2:
                cl.append([-x, -A, sij])
            # D1
            if A is not TRUE:
                cl.append([-sij, -x, A])
            # D2
            if B is FALSE:
                cl.append([-sij, x])
            else:
                cl.append([-sij, x, B])
    cl.append([-s[(m, k + 1)]])
    return cl, next_var


def write_cnf(path, clauses, nvars):
    with open(path, "w") as f:
        f.write("p cnf %d %d\n" % (nvars, len(clauses)))
        for c in clauses:
            f.write(" ".join(str(x) for x in c) + " 0\n")


def selftest(solver="./kissat", tmpdir="/tmp", mmax=7, kmax=5):
    ok = True
    for m in range(1, mmax + 1):
        for k in range(0, min(m, kmax)):
            lits = list(range(1, m + 1))
            clauses, nv = at_most_k(lits, k, m + 1)
            bad = 0
            for bits in itertools.product([0, 1], repeat=m):
                extra = [[(i + 1)] if b else [-(i + 1)] for i, b in enumerate(bits)]
                cn = clauses + extra
                path = os.path.join(tmpdir, "card2_test.cnf")
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
