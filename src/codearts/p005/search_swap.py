import random
import sys
sys.path.insert(0, '/workspace/few-shots-work/p005')
import capset as cs

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'


def witness_map(S):
    w = {}
    L = sorted(S)
    for i, a in enumerate(L):
        for b in L[i + 1:]:
            w.setdefault(cs.third(a, b), []).append((a, b))
    return w


def attempt(S, W, rng, keep_limit=None):
    ext = [x for x in W if x not in S]
    ext.sort(key=lambda x: len(W[x]))
    if keep_limit:
        ext = ext[:keep_limit]
    x = rng.choice(ext)
    lines = W[x]
    R = set()
    order = list(lines)
    rng.shuffle(order)
    for a, b in order:
        if a in R or b in R:
            continue
        R.add(a if rng.random() < 0.5 else b)
    S2 = (S - R) | {x}
    A = cs.available(S2)
    out = cs.repair(S2, A, rng)
    return out, x, R


def main():
    S = cs.load_points(BASE)
    W = witness_map(S)
    rng = random.Random(12345)
    best = 512
    bestS = None
    for t in range(3000):
        out, x, R = attempt(S, W, rng, keep_limit=rng.choice([40, 120, 400, None]))
        n = len(out)
        if n > best:
            best = n
            bestS = set(out)
            cs.save_points('/workspace/few-shots-work/p005/best.json', out)
            print('iter', t, 'new best', best, 'x', x, 'removed', len(R), flush=True)
            if best >= 513:
                return
    print('final best', best)


if __name__ == '__main__':
    main()