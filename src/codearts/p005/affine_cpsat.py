import json
import random
import sys
import time

from ortools.sat.python import cp_model

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'
D7 = layers.T7
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


def build_lines():
    lines = []
    for a in range(D7):
        for b in range(a + 1, D7):
            c = layers.third7(a, b)
            if c > b:
                lines.append((a, b, c))
    return lines


def solve_layers(Afix, B, C, lines, time_limit=600):
    t = time.time()
    model = cp_model.CpModel()
    y = [model.NewBoolVar(f'y{b}') for b in range(D7)]
    z = [model.NewBoolVar(f'z{b}') for b in range(D7)]
    for (a, b, c) in lines:
        model.Add(y[a] + y[b] + y[c] <= 2)
        model.Add(z[a] + z[b] + z[c] <= 2)
    for a in Afix:
        for b in range(D7):
            c = layers.third7(a, b)
            model.Add(y[b] + z[c] <= 1)
    model.Maximize(sum(y) + sum(z))
    for b in B:
        model.AddHint(y[b], 1)
    for b in C:
        model.AddHint(z[b], 1)
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = time_limit
    solver.parameters.num_search_workers = 2
    print('model built', round(time.time() - t, 1), 'solving', flush=True)
    status = solver.Solve(model)
    print('status', solver.StatusName(status), 'obj', solver.ObjectiveValue(), 'wall', solver.WallTime(), flush=True)
    if status in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        NY = {b for b in range(D7) if solver.Value(y[b])}
        NZ = {b for b in range(D7) if solver.Value(z[b])}
        return len(NY) + len(NZ), NY, NZ
    return None, None, None


def main(seed=1):
    with open(BASE) as f:
        P = json.load(f)['points']
    rng = random.Random(seed)
    lines = build_lines()
    for attempt in range(6):
        M = [[rng.randrange(3) for _ in range(N)] for _ in range(N)]
        Minv = inverse(M)
        if Minv is None:
            continue
        Y = [apply(M, x) for x in P]
        groups = {0: set(), 1: set(), 2: set()}
        for y in Y:
            groups[y[0]].add(encode7(y[1:]))
        order = sorted(range(3), key=lambda i: -len(groups[i]))
        fix = order[0]
        i, j = order[1], order[2]
        Afix, B, C = groups[fix], groups[i], groups[j]
        print('attempt', attempt, 'sizes fix/i/j', len(Afix), len(B), len(C), 'sum', len(Afix) + len(B) + len(C), flush=True)
        target = len(Afix) + len(B) + len(C)
        total, NY, NZ = solve_layers(Afix, B, C, lines, time_limit=600)
        if total is not None:
            print('DONE total', total, 'need >', target, flush=True)
            if total > target:
                pts = []
                for b in Afix:
                    pts.append([fix] + layers.decode7(b))
                for b in NY:
                    pts.append([i] + layers.decode7(b))
                for b in NZ:
                    pts.append([j] + layers.decode7(b))
                out = [apply(Minv, y) for y in pts]
                with open('/workspace/few-shots-work/p005/affine_cpsat_candidate.json', 'w') as f:
                    json.dump({'points': out}, f)
                print('SAVED', total, flush=True)
                return
        print('attempt', attempt, 'no improvement', flush=True)


if __name__ == '__main__':
    main(int(sys.argv[1]) if len(sys.argv) > 1 else 1)