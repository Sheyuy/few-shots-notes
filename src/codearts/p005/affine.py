import json
import random
import sys

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers
import layers_exact

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'
N = 8


def mat_mul(A, B):
    C = [[0] * N for _ in range(N)]
    for i in range(N):
        for k in range(N):
            if A[i][k]:
                a = A[i][k]
                for j in range(N):
                    C[i][j] = (C[i][j] + a * B[k][j]) % 3
    return C


def inverse(A):
    aug = [row[:] + [1 if i == j else 0 for j in range(N)] for i, row in enumerate(A)]
    for col in range(N):
        piv = next((r for r in range(col, N) if aug[r][col]), None)
        if piv is None:
            return None
        aug[col], aug[piv] = aug[piv], aug[col]
        inv = 1 if aug[col][col] == 1 else 2
        aug[col] = [(v * inv) % 3 for v in aug[col]]
        for r in range(N):
            if r != col and aug[r][col]:
                f = aug[r][col]
                aug[r] = [(aug[r][c] - f * aug[col][c]) % 3 for c in range(2 * N)]
    return [row[N:] for row in aug]


def apply(M, x):
    return [sum(M[i][j] * x[j] for j in range(N)) % 3 for i in range(N)]


def encode7(p):
    x = 0
    for v in p:
        x = x * 3 + v
    return x


def run(M, P, rng=None, rounds=2):
    Minv = inverse(M)
    if Minv is None:
        return None
    Y = [apply(M, x) for x in P]
    L = {}
    for y in Y:
        L.setdefault(y[0], set()).add(encode7(y[1:]))
    if any(len(L.get(i, ())) == 0 for i in range(3)):
        return None
    for _ in range(rounds):
        changed = False
        for i in range(3):
            C = layers.candidates(L, i)
            if not L[i] <= C:
                return None
            sol, n, k, nodes, dt = layers_exact.solve(C, init=L[i], limit=15)
            if len(sol) > len(L[i]):
                L[i] = sol
                changed = True
        if not changed:
            break
    total = sum(len(L[i]) for i in range(3))
    pts = []
    for i in range(3):
        for x in L[i]:
            y = [i] + layers.decode7(x)
            pts.append(apply(Minv, y))
    return total, pts


def main():
    with open(BASE) as f:
        P = json.load(f)['points']
    rng = random.Random(4242)
    best = 512
    for t in range(120):
        M = [[rng.randrange(3) for _ in range(N)] for _ in range(N)]
        r = run(M, P)
        if r is None:
            continue
        total, pts = r
        if total > best:
            best = total
            with open('/workspace/few-shots-work/p005/affine_best.json', 'w') as f:
                json.dump({'points': pts}, f)
            print('iter', t, 'BEST', best, 'M', M, flush=True)
            if best >= 513:
                return
        if t % 10 == 0:
            print('iter', t, 'cur', total, 'best', best, flush=True)
    print('final', best)


if __name__ == '__main__':
    main()