"""Extract the sorting network from a kissat model of a sortsat.c instance and
verify it independently (exhaustive over all 2^n inputs).

Usage: extract_net.py <model.txt> <n> <d> <out.json>
"""
import sys
import json


def pidx(i, j, n):
    return i * (2 * n - i - 1) // 2 + (j - i - 1)


def main():
    model_f, n, d, out_f = sys.argv[1:5]
    n = int(n); d = int(d)
    NP = n * (n - 1) // 2
    base_g = 1

    true_vars = set()
    with open(model_f) as f:
        for line in f:
            if not line.startswith('v '):
                continue
            for tok in line.split()[1:]:
                t = int(tok)
                if t > 0:
                    true_vars.add(t)

    comps = []
    for k in range(d):
        for i in range(n):
            for j in range(i + 1, n):
                if base_g + k * NP + pidx(i, j, n) in true_vars:
                    comps.append((i, j))

    # independent exhaustive check of the 0/1 principle
    bad = 0
    for x in range(1 << n):
        w = [(x >> i) & 1 for i in range(n)]
        for (a, b) in comps:
            if w[a] > w[b]:
                w[a], w[b] = w[b], w[a]
        if any(w[i] > w[i + 1] for i in range(n - 1)):
            bad += 1
    print("comparators: %d | inputs not sorted: %d" % (len(comps), bad))
    if bad == 0:
        json.dump({"comparators": [[a, b] for (a, b) in comps]}, open(out_f, 'w'))
        print("OK: verified sorting network ->", out_f)
    else:
        print("NOT a sorting network")


if __name__ == '__main__':
    main()
