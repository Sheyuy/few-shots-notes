import json
import random
import sys

import numpy as np

sys.path.insert(0, '/workspace/few-shots-work/p001')
import norm_check as nc

M = 72


def compat(Xp, Xq, P, Q):
    A = 2 * (Xp @ P.T + 2 * (Xq @ Q.T)) - M
    B = 2 * (Xp @ Q.T + Xq @ P.T)
    inv = ((A > 0) & (B >= 0)) | ((A > 0) & (B < 0) & (A * A > 2 * B * B)) | ((A <= 0) & (B > 0) & (2 * B * B > A * A))
    return ~inv.any(axis=1)


def main():
    reps = nc.reps_table(M)
    dp = nc.dp_table(reps, M)
    rng = random.Random(888)
    pool = [nc.sample(dp, reps, M, rng) for _ in range(20000)]
    print('pool', len(pool), flush=True)
    S = []
    Sp = np.zeros((0, 11), dtype=np.int64)
    Sq = np.zeros((0, 11), dtype=np.int64)
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
        if start % 5000 == 0:
            print('at', start, 'size', len(S), flush=True)
    print('GREEDY SIZE', len(S), flush=True)
    with open('/workspace/few-shots-work/p001/zero2.json', 'w') as f:
        json.dump({'vectors': [[[p, q] for p, q in x] for x in S]}, f)


if __name__ == '__main__':
    main()