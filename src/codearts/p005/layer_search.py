import json
import random
import sys

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers
import layers_exact

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'


def full_candidates(i, L):
    j = (i + 1) % 3
    k = (i + 2) % 3
    Sj, Sk = L[j], L[k]
    out = set()
    for x in range(layers.T7):
        ok = True
        for b in Sj:
            if layers.third7(x, b) in Sk:
                ok = False
                break
        if ok:
            for b in Sk:
                if layers.third7(x, b) in Sj:
                    ok = False
                    break
        if ok:
            out.add(x)
    return out


def to_points(L):
    pts = []
    for i in range(3):
        for x in L[i]:
            pts.append([i] + layers.decode7(x))
    return pts


def try_once(S, rng, a=2, b=2):
    S0, S1, S2 = S[0], S[1], S[2]
    R2 = set(rng.sample(sorted(S2), b))
    S2n = S2 - R2
    C1 = full_candidates(1, {0: S0, 1: S1, 2: S2n})
    S1n, n1, k1, _, _ = layers_exact.solve(C1, init=S1, limit=20)
    if len(S1n) < len(S1) - b - 2:
        return None
    R1 = set(rng.sample(sorted(S1n), a))
    S1m = S1n - R1
    C2 = full_candidates(2, {0: S0, 1: S1m, 2: S2n})
    S2m, n2, k2, _, _ = layers_exact.solve(C2, init=S2n, limit=20)
    total = len(S0) + len(S1n) + len(S2m)
    return {0: S0, 1: S1n, 2: S2m}, total


def main():
    L = layers.load()
    rng = random.Random(31337)
    best = sum(len(L[i]) for i in range(3))
    print('start', best, [len(L[i]) for i in range(3)], flush=True)
    for t in range(300):
        a = rng.choice([1, 2, 3, 3, 4])
        b = rng.choice([1, 2, 3, 3, 4])
        r = try_once(L, rng, a, b)
        if r is None:
            continue
        L2, total = r
        if total > best:
            best = total
            with open('/workspace/few-shots-work/p005/layer_best.json', 'w') as f:
                json.dump({'points': to_points(L2)}, f)
            print('iter', t, 'best', best, 'sizes', [len(L2[i]) for i in range(3)], flush=True)
            if best >= 513:
                return
        if t % 10 == 0:
            print('iter', t, 'cur', total, 'best', best, flush=True)
    print('final', best)


if __name__ == '__main__':
    main()