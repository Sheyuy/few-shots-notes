"""Independent brute-force checker for the sortsat CNF.

Parses the DIMACS file, then searches all assignments of the *comparator*
variables g (and lets the rest be determined) is too big, so instead we do
the opposite: build the network from a candidate g-assignment, evaluate it
directly, and separately verify that the CNF clauses are satisfied by the
assignment the solver reports.  The second part is what we need to find
encoding bugs: it tells us WHICH clause is violated.
"""
import sys


def load(path):
    cls = []
    with open(path) as f:
        for line in f:
            if line.startswith('c') or line.startswith('p'):
                continue
            lits = [int(x) for x in line.split()]
            if lits and lits[-1] == 0:
                lits = lits[:-1]
            if lits:
                cls.append(lits)
    return cls


def violated(cls, assign, nvars):
    """Return the index of the first clause not satisfied by `assign`
    (a dict var -> True/False), or -1."""
    for i, c in enumerate(cls):
        ok = False
        for l in c:
            v = assign.get(abs(l))
            if v is None:
                continue
            if (l > 0) == bool(v):
                ok = True
                break
        if not ok:
            return i
    return -1


def main():
    cnf = sys.argv[1]
    model = sys.argv[2]
    cls = load(cnf)
    assign = {}
    for line in open(model):
        if not line.startswith('v '):
            continue
        for tok in line.split()[1:]:
            t = int(tok)
            if t > 0:
                assign[t] = True
            elif t < 0:
                assign[-t] = False
    bad = violated(cls, assign, 0)
    if bad < 0:
        print("model satisfies every clause")
    else:
        print("VIOLATED clause #%d:" % bad, cls[bad])
        # which variables in that clause are unassigned?
        for l in cls[bad]:
            if abs(l) not in assign:
                print("   unassigned var", l)


if __name__ == '__main__':
    main()
