"""Build a full variable assignment for a given layered network, mirroring the
variable layout of sortsat.c, then check it against the generated CNF.

The assignment is built from the SEMANTICS of the network (simulate it), not
from the encoding, so a violated clause pinpoints a bug in sortsat.c.

Usage:  mkassign.py <cnf> <prefix.txt> <inputs.txt> <n> <d> <maxC> <out.txt>
The prefix file gives one layer per line, pairs of 0-based channel indices.
"""
import sys


def pidx(i, j, n):
    return i * (2 * n - i - 1) // 2 + (j - i - 1)


def main():
    cnf, pref_f, inp_f, n, d, maxC, out_f = sys.argv[1:8]
    n = int(n); d = int(d); maxC = int(maxC)
    NP = n * (n - 1) // 2

    # ---- read the prefix (layers) ----------------------------------------
    layers = []
    with open(pref_f) as f:
        for line in f:
            if line.startswith('#'):
                layers.append([])
                continue
            vals = [int(x) for x in line.split()]
            layers.append([(vals[i], vals[i + 1]) for i in range(0, len(vals) - 1, 2)])
    while len(layers) < d:
        layers.append([])

    # ---- read the inputs --------------------------------------------------
    inputs = [int(x) for x in open(inp_f).read().split()]
    ninp = len(inputs)

    # ---- variable layout (must match sortsat.c) ---------------------------
    base_g = 1
    G = NP * d
    base_u = base_g + G
    base_v = base_u + n * d
    base_s = base_v + ninp * n * (d + 1)
    nvars = base_s + G * (maxC + 1) - 1

    A = {}                     # var -> bool

    # ---- g: which comparator slots are occupied ---------------------------
    for k in range(d):
        for i in range(n):
            for j in range(i + 1, n):
                A[base_g + k * NP + pidx(i, j, n)] = False
    for k in range(d):
        for (i, j) in layers[k]:
            A[base_g + k * NP + pidx(min(i, j), max(i, j), n)] = True

    # ---- u: channel touched in a layer ------------------------------------
    for k in range(d):
        for i in range(n):
            touched = any(i in p for p in layers[k])
            A[base_u + k * n + i] = touched

    # ---- v: simulate the network ------------------------------------------
    for m, x in enumerate(inputs):
        w = [(x >> i) & 1 for i in range(n)]
        for i in range(n):
            A[base_v + (m * n + i) * (d + 1) + 0] = bool(w[i])
        for k in range(d):
            for (a, b) in layers[k]:
                if w[a] > w[b]:
                    w[a], w[b] = w[b], w[a]
            for i in range(n):
                A[base_v + (m * n + i) * (d + 1) + (k + 1)] = bool(w[i])

    # ---- Sinz counter: s[i][j] = "at least j+1 of x_0..x_i are true" -------
    cnt = 0
    for i in range(G):
        if A.get(base_g + i, False):
            cnt += 1
        for j in range(1, maxC + 1):
            A[base_s + i * (maxC + 1) + j] = (cnt >= j)

    with open(out_f, 'w') as f:
        for v in range(1, nvars + 1):
            f.write(("v " if A.get(v, False) else "v -") + str(v) + "\n")
    print("assignment written: %d vars, %d true" % (nvars, sum(1 for x in A.values() if x)))


if __name__ == '__main__':
    main()
