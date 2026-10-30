import json
import random

N = 8
HALF = 4
M = 3 ** HALF
TOTAL = 3 ** N


def build_third():
    th = [[0] * M for _ in range(M)]
    for u in range(M):
        for v in range(M):
            w = 0
            place = 1
            a, b = u, v
            for _ in range(HALF):
                w += (-(a % 3 + b % 3)) % 3 * place
                a //= 3
                b //= 3
                place *= 3
            th[u][v] = w
    return th


TH = build_third()


def third(a, b):
    return TH[a // M][b // M] * M + TH[a % M][b % M]


def encode(p):
    x = 0
    for v in p:
        x = x * 3 + v
    return x


def decode(x):
    c = [0] * N
    for i in range(N - 1, -1, -1):
        c[i] = x % 3
        x //= 3
    return c


def load_points(path):
    with open(path) as f:
        data = json.load(f)
    return {encode(p) for p in data["points"]}


def save_points(path, S):
    pts = sorted(decode(x) for x in S)
    with open(path, "w") as f:
        json.dump({"points": pts}, f, separators=(",", ":"))


def forbidden(S):
    F = set()
    for a in S:
        for b in S:
            if a != b:
                F.add(third(a, b))
    return F


def available(S):
    return set(range(TOTAL)) - S - forbidden(S)


def add_point(S, A, x):
    S.add(x)
    A.discard(x)
    for y in S:
        A.discard(third(x, y))


def repair(S, A, rng):
    S = set(S)
    A = set(A)
    order = list(A)
    rng.shuffle(order)
    for x in order:
        if x in A:
            add_point(S, A, x)
    return S


def is_cap(S):
    for a in S:
        for b in S:
            if a != b and third(a, b) in S:
                return False
    return True