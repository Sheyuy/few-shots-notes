import itertools
import json
import numpy as np

DIM = 11
NORM = 36
HALF = 18
BASE = "/workspace/few-shots/problems/p001-kissing-number-11d/baseline/certificate.json"


def load_base():
    with open(BASE) as f:
        data = json.load(f)
    P = np.array([[c[0] for c in v] for v in data["vectors"]], dtype=np.int64)
    Q = np.array([[c[1] for c in v] for v in data["vectors"]], dtype=np.int64)
    return P, Q


def gen_family(kind):
    out = []
    positions = range(DIM)
    if kind == "a":
        for S in itertools.combinations(positions, 4):
            for signs in itertools.product((3, -3), repeat=4):
                p = [0] * DIM
                for pos, s in zip(S, signs):
                    p[pos] = s
                out.append((p, [0] * DIM))
    elif kind == "b":
        for pos in positions:
            for s in (6, -6):
                p = [0] * DIM
                p[pos] = s
                out.append((p, [0] * DIM))
    elif kind == "c":
        for P2 in itertools.combinations(positions, 2):
            rest = [i for i in positions if i not in P2]
            for Q3 in itertools.combinations(rest, 3):
                for psigns in itertools.product((3, -3), repeat=2):
                    for one in range(3):
                        for qsigns in itertools.product((1, 2, 2), repeat=1):
                            pass
                    for which in range(3):
                        for qsigns in itertools.product((1, -1), repeat=3):
                            p = [0] * DIM
                            for pos, s in zip(P2, psigns):
                                p[pos] = s
                            q = [0] * DIM
                            vals = [1, 2, 2]
                            vals[0], vals[which] = vals[which], vals[0]
                            for pos, s, v in zip(Q3, qsigns, vals):
                                q[pos] = s * v
                            out.append((p, q))
    elif kind == "c2":
        for P2 in itertools.combinations(positions, 2):
            rest = [i for i in positions if i not in P2]
            for qpos in rest:
                for psigns in itertools.product((3, -3), repeat=2):
                    for qs in (3, -3):
                        p = [0] * DIM
                        for pos, s in zip(P2, psigns):
                            p[pos] = s
                        q = [0] * DIM
                        q[qpos] = qs
                        out.append((p, q))
    elif kind == "d":
        for Q3 in itertools.combinations(positions, 3):
            for which in range(3):
                for qsigns in itertools.product((1, -1), repeat=3):
                    q = [0] * DIM
                    vals = [4, 1, 1]
                    vals[0], vals[which] = vals[which], vals[0]
                    for pos, s, v in zip(Q3, qsigns, vals):
                        q[pos] = s * v
                    out.append(([0] * DIM, q))
    elif kind == "e":
        for Q2 in itertools.combinations(positions, 2):
            for qsigns in itertools.product((3, -3), repeat=2):
                q = [0] * DIM
                for pos, s in zip(Q2, qsigns):
                    q[pos] = s
                out.append(([0] * DIM, q))
    return out


def check(cands, P, Q):
    Xp = np.array([c[0] for c in cands], dtype=np.int64)
    Xq = np.array([c[1] for c in cands], dtype=np.int64)
    good = []
    chunk = 50000
    for start in range(0, len(Xp), chunk):
        xp = Xp[start:start + chunk]
        xq = Xq[start:start + chunk]
        A = 2 * (xp @ P.T + 2 * (xq @ Q.T)) - NORM
        B = 2 * (xp @ Q.T + xq @ P.T)
        inv = ((A > 0) & (B >= 0)) | ((A > 0) & (B < 0) & (A * A > 2 * B * B)) | ((A <= 0) & (B > 0) & (2 * B * B > A * A))
        ok = ~inv.any(axis=1)
        for idx in np.nonzero(ok)[0]:
            good.append((list(xp[idx]), list(xq[idx])))
    return good


if __name__ == "__main__":
    P, Q = load_base()
    for kind in ("a", "b", "c", "c2", "d", "e"):
        cands = gen_family(kind)
        good = check(cands, P, Q)
        print(kind, "candidates", len(cands), "valid additions", len(good), good[:5])