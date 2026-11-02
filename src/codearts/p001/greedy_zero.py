import json
import math
import random
import sys

import numpy as np

sys.path.insert(0, '/workspace/few-shots-work/p001')
import norm_check as nc

M = 72
HALF = M // 2


def compatible(Xp, Xq, P, Q):
    A = 2 * (Xp @ P.T + 2 * (Xq @ Q.T)) - M
    B = 2 * (Xp @ Q.T + Xq @ P.T)
    inv = ((A > 0) & (B >= 0)) | ((A > 0) & (B < 0) & (A * A > 2 * B * B)) | ((A <= 0) & (B > 0) & (2 * B * B > A * A))
    return ~inv.any(axis=1)


def main():
    reps = nc.reps_table(M)
    dp = nc.dp_table(reps, M)
    rng = random.Random(555)
    pool = []
    seen = set()
    while len(pool) < 60000:
        x = nc.sample(dp, reps, M, rng)
        key = tuple(x)
        if key in seen:
            continue
        seen.add(key)
        pool.append(x)
    print('pool', len(pool), flush=True)
    S = []
    Sp = np.zeros((0, 11), dtype=np.int64)
    Sq = np.zeros((0, 11), dtype=np.int64)
    for start in range(0, len(pool), 2000):
        batch = pool[start:start + 2000]
        Xp = np.array([[p for p, q in x] for x in batch], dtype=np.int64)
        Xq = np.array([[q for p, q in x] for x in batch], dtype=np.int64)
        if len(S):
            mask = compatible(Xp, Xq, Sp, Sq)
        else:
            mask = np.ones(len(batch), dtype=bool)
        cand = [batch[i] for i in np.nonzero(mask)[0]]
        for x in cand:
            xp = np.array([[p for p, q in x]], dtype=np.int64)
            xq = np.array([[q for p, q in x]], dtype=np.int64)
            if len(S) and not compatible(xp, xq, Sp, Sq)[0]:
                continue
            S.append(x)
            Sp = np.vstack([Sp, xp])
            Sq = np.vstack([Sq, xq])
        if start % 10000 == 0:
            print('pool at', start, 'size', len(S), flush=True)
    print('FINAL', len(S), flush=True)
    with open('/workspace/few-shots-work/p001/greedy_zero.json', 'w') as f:
        json.dump({'vectors': [[[p, q] for p, q in x] for x in S]}, f)


if __name__ == '__main__':
    main()