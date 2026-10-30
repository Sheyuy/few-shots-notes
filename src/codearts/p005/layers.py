import json
import sys

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'
D7 = 7
HALF = 3
M = 3 ** HALF
T7 = 3 ** D7


def build_third(half):
    m = 3 ** half
    th = [[0] * m for _ in range(m)]
    for u in range(m):
        for v in range(m):
            w = 0
            place = 1
            a, b = u, v
            for _ in range(half):
                w += (-(a % 3 + b % 3)) % 3 * place
                a //= 3
                b //= 3
                place *= 3
            th[u][v] = w
    return th


def split(dim):
    h = dim // 2
    m = 3 ** h
    th = build_third(h)
    return h, m, th


TH3 = build_third(3)
TH4 = build_third(4)
M4 = 81


def third7(a, b):
    return TH3[a // M4][b // M4] * M4 + TH4[a % M4][b % M4]


def decode7(x):
    c = [0] * 7
    for i in range(6, -1, -1):
        c[i] = x % 3
        x //= 3
    return c


def encode(p):
    x = 0
    for v in p:
        x = x * 3 + v
    return x


def load():
    with open(BASE) as f:
        pts = json.load(f)['points']
    layers = {}
    for p in pts:
        layers.setdefault(p[0], set()).add(encode(p[1:]))
    return layers


def candidates(layers, i):
    S1 = layers[(i + 1) % 3]
    S2 = layers[(i + 2) % 3]
    out = set()
    for x in range(T7):
        ok = True
        for b in S1:
            if third7(x, b) in S2:
                ok = False
                break
        if ok:
            for b in S2:
                if third7(x, b) in S1:
                    ok = False
                    break
        if ok:
            out.add(x)
    return out


def max_cap(C):
    L = sorted(C)
    idx = {x: i for i, x in enumerate(L)}
    n = len(L)
    adj = [0] * n
    for i in range(n):
        a = L[i]
        m = 0
        for j in range(n):
            if i != j and third7(a, L[j]) not in C:
                m |= 1 << j
        adj[i] = m
    best = []

    def expand(R, P):
        nonlocal best
        if not P:
            if len(R) > len(best):
                best = list(R)
            return
        if len(R) + bin(P).count('1') <= len(best):
            return
        while P:
            v = (P & -P).bit_length() - 1
            P &= P - 1
            expand(R + [v], P & adj[v])

    expand([], (1 << n) - 1)
    return {L[i] for i in best}, n


def main():
    layers = load()
    for i in (0, 1, 2):
        C = candidates(layers, i)
        cur = layers[i]
        assert cur <= C, (i, len(C), len(cur))
        print('layer', i, 'current', len(cur), 'candidates', len(C), flush=True)
        best, n = max_cap(C)
        print('layer', i, 'max in candidates', len(best), 'graph n', n, flush=True)
        if len(best) > len(cur):
            layers[i] = best
            print('IMPROVED layer', i, 'to', len(best), flush=True)


if __name__ == '__main__':
    main()