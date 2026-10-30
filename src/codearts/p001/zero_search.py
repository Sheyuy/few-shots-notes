import json
import random
import sys

import numpy as np

M = 72
N = 11
HALF = M // 2


def build_reps(m):
    reps = []
    for p in range(-7, 8):
        for q in range(-6, 7):
            w = p * p + 2 * q * q
            if 0 < w <= m:
                reps.append((w, p, q))
    return reps


def gen(rng, reps):
    by_w = {}
    for w, p, q in reps:
        by_w.setdefault(w, []).append((p, q))
    for _ in range(200):
        coords = []
        s = 0
        ok = True
        for i in range(N - 1):
            w, p, q = rng.choice(reps)
            if s + w > M:
                ok = False
                break
            coords.append((p, q))
            s += w
        if not ok:
            continue
        rem = M - s
        if rem == 0:
            coords.append((0, 0))
        elif rem in by_w:
            coords.append(rng.choice(by_w[rem]))
        else:
            continue
        return coords
    return None


def compat(Xp, Xq, P, Q):
    A = 2 * (Xp @ P.T + 2 * (Xq @ Q.T)) - M
    B = 2 * (Xp @ Q.T + Xq @ P.T)
    inv = ((A > 0) & (B >= 0)) | ((A > 0) & (B < 0) & (A * A > 2 * B * B)) | ((A <= 0) & (B > 0) & (2 * B * B > A * A))
    return ~inv.any(axis=1)


def main():
    reps = build_reps(M)
    rng = random.Random(777)
    pool = []
    seen = set()
    tries = 0
    while len(pool) < 30000 and tries < 200000:
        tries += 1
        x = gen(rng, reps)
        if x is None:
            continue
        key = tuple(x)
        if key in seen:
            continue
        seen.add(key)
        pool.append(x)
    print('pool', len(pool), 'tries', tries, flush=True)
    S = []
    Sp = np.zeros((0, N), dtype=np.int64)
    Sq = np.zeros((0, N), dtype=np.int64)
    for start in range(0, len(pool), 1000):
        batch = pool[start:start + 1000]
        Xp = np.array([[p for p, q in x] for x in batch], dtype=np.int64)
        Xq = np.array([[q for p, q in x] for x in batch], dtype=np.int64)
        cand = batch if not len(S) else [batch[i] for i in np.nonzero(compat(Xp, Xq, Sp, Sq))[0]]
        for x in cand:
            xp = np.array([[p for p, q in x]], dtype=np.int64)
            xq = np.array([[q for p, q in x]], dtype=np.int64)
            if len(S) and not compat(xp, xq, Sp, Sq)[0]:
                continue
            S.append(x)
            Sp = np.vstack([Sp, xp])
            Sq = np.vstack([Sq, xq])
    print('GREEDY SIZE', len(S), flush=True)
    with open('/workspace/few-shots-work/p001/zero.json', 'w') as f:
        json.dump({'vectors': [[[p, q] for p, q in x] for x in S]}, f)


if __name__ == '__main__':
    main()