import random
import sys
sys.path.insert(0, '/workspace/few-shots-work/p005')
import capset as cs

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'


def lines_of(S):
    L = sorted(S)
    out = []
    for i, a in enumerate(L):
        for b in L[i + 1:]:
            c = cs.third(a, b)
            if c in S:
                out.append((a, b, c))
    return out


def resolve(S, rng, protect=()):
    S = set(S)
    prot = set(protect)
    for _ in range(200):
        L = sorted(S)
        conflict = None
        for i, a in enumerate(L):
            for b in L[i + 1:]:
                if cs.third(a, b) in S:
                    conflict = (a, b, cs.third(a, b))
                    break
            if conflict:
                break
        if not conflict:
            return S
        cand = [x for x in conflict if x not in prot]
        if not cand:
            cand = list(conflict)
        S.discard(rng.choice(cand))
    return S


def main():
    S = cs.load_points(BASE)
    rng = random.Random(999)
    W = {}
    L = sorted(S)
    for i, a in enumerate(L):
        for b in L[i + 1:]:
            W.setdefault(cs.third(a, b), []).append((a, b))
    ext = [x for x in W if x not in S]
    ext.sort(key=lambda x: len(W[x]))
    best = 512
    for t in range(400):
        m = rng.choice([2, 3, 4, 6])
        pool = ext[:rng.choice([60, 200, 600, len(ext)])]
        X = rng.sample(pool, m)
        S2 = S | set(X)
        S2 = resolve(S2, rng, protect=X)
        A = cs.available(S2)
        out = cs.repair(S2, A, rng)
        n = len(out)
        if n > best:
            best = n
            cs.save_points('/workspace/few-shots-work/p005/best_multi.json', out)
            print('iter', t, 'best', best, 'm', m, flush=True)
            if best >= 513:
                return
        if t % 20 == 0:
            print('iter', t, 'cur', n, 'best', best, flush=True)
    print('final', best)


if __name__ == '__main__':
    main()