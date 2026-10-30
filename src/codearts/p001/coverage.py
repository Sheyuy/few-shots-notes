import json
import math
import sys

import numpy as np

BASE = '/workspace/few-shots/problems/p001-kissing-number-11d/baseline/certificate.json'


def load():
    with open(BASE) as f:
        c = json.load(f)
    V = np.array([[p + q * math.sqrt(2) for p, q in v] for v in c['vectors']], dtype=np.float64)
    V /= np.linalg.norm(V, axis=1, keepdims=True)
    return V


def descend(V, u, iters=4000, lr=0.03):
    best = 9.0
    for t in range(iters):
        d = V @ u
        i = int(np.argmax(d))
        best = min(best, float(d[i]))
        if d[i] <= 0.5 + 1e-10:
            return float(d[i]), u
        g = V[i] - d[i] * u
        u = u - lr * g
        n = np.linalg.norm(u)
        if n < 1e-12:
            break
        u /= n
        lr *= 0.9993
    return best, u


def main():
    V = load()
    rng = np.random.default_rng(2026)
    best = (9.0, None)
    for j in range(150):
        if j % 3 == 0:
            u = rng.normal(size=11)
        elif j % 3 == 1:
            k = rng.integers(2, 6)
            idx = rng.choice(604, size=k, replace=False)
            s = rng.choice([-1.0, 1.0], size=k)
            u = (V[idx] * s[:, None]).sum(axis=0)
        else:
            u = rng.normal(size=11) * 0.1 + V[rng.integers(604)]
        n = np.linalg.norm(u)
        if n < 1e-12:
            continue
        u /= n
        f, u2 = descend(V, u)
        if f < best[0]:
            best = (f, u2.copy())
            print('restart', j, 'min max-dot', f, flush=True)
    print('BEST', best[0])
    print(list(best[1]))
    print('angle deg', math.degrees(math.acos(best[0])))


if __name__ == '__main__':
    main()