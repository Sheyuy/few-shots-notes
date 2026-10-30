import json
import math
import random
import sys

import numpy as np

BASE = '/workspace/few-shots/problems/p001-kissing-number-11d/baseline/certificate.json'
M = 72
HALF = M // 2
N = 11


def reps_table(m):
    reps = {}
    for p in range(-math.isqrt(m), math.isqrt(m) + 1):
        for q in range(-math.isqrt(m // 2), math.isqrt(m // 2) + 1):
            w = p * p + 2 * q * q
            if w <= m:
                reps.setdefault(w, []).append((p, q))
    return reps


def dp_table(reps, m, n=N):
    ws = sorted(reps)
    dp = [{} for _ in range(n + 1)]
    dp[n][0] = 1
    for i in range(n - 1, -1, -1):
        cur = {}
        for s in range(m + 1):
            tot = 0
            for w in ws:
                if s - w >= 0:
                    v = dp[i + 1].get(s - w, 0)
                    if v:
                        tot += v * len(reps[w])
            if tot:
                cur[s] = tot
        dp[i] = cur
    return dp


def sample(dp, reps, m, rng, n=N):
    ws = sorted(reps)
    s = m
    coords = []
    for i in range(n):
        opts = []
        for w in ws:
            if s - w >= 0:
                v = dp[i + 1].get(s - w, 0) * len(reps[w])
                if v:
                    opts.append((w, v))
        tot = sum(v for _, v in opts)
        r = rng.randrange(tot)
        chosen = None
        for w, v in opts:
            if r < v:
                chosen = w
                break
            r -= v
        coords.append(rng.choice(reps[chosen]))
        s -= chosen
    return coords


def load_scaled():
    with open(BASE) as f:
        c = json.load(f)
    P, Q = [], []
    for v in c['vectors']:
        P.append([2 * q for p, q in v])
        Q.append([p for p, q in v])
    return np.array(P, dtype=np.int64), np.array(Q, dtype=np.int64)


def valid_mask(Xp, Xq, P, Q):
    A = 2 * (Xp @ P.T + 2 * (Xq @ Q.T)) - M
    B = 2 * (Xp @ Q.T + Xq @ P.T)
    inv = ((A > 0) & (B >= 0)) | ((A > 0) & (B < 0) & (A * A > 2 * B * B)) | ((A <= 0) & (B > 0) & (2 * B * B > A * A))
    return ~inv.any(axis=1)


def main():
    reps = reps_table(M)
    print('reps weights', len(reps), flush=True)
    dp = dp_table(reps, M)
    print('total solutions', dp[0].get(M), flush=True)
    rng = random.Random(2027)
    P, Q = load_scaled()
    total = 0
    ok = 0
    for _ in range(20):
        X = [sample(dp, reps, M, rng) for _ in range(5000)]
        Xp = np.array([[p for p, q in x] for x in X], dtype=np.int64)
        Xq = np.array([[q for p, q in x] for x in X], dtype=np.int64)
        mask = valid_mask(Xp, Xq, P, Q)
        mask = mask & (2 * (Xp * Xq).sum(axis=1) == 0)
        total += len(X)
        ok += int(mask.sum())
        print('batch valid', int(mask.sum()), flush=True)
        if ok:
            idx = int(np.nonzero(mask)[0][0])
            print('FOUND', X[idx], flush=True)
            return
    print('total', total, 'valid', ok, flush=True)


if __name__ == '__main__':
    main()