import json
import sys
import time

from ortools.sat.python import cp_model

D = 8
T = 3 ** D
H = 4
M = 3 ** H
TH = [[0] * M for _ in range(M)]
for u in range(M):
    for v in range(M):
        w = 0
        p = 1
        a, b = u, v
        for _ in range(H):
            w += (-(a % 3 + b % 3)) % 3 * p
            a //= 3
            b //= 3
            p *= 3
        TH[u][v] = w


def third(a, b):
    return TH[a // M][b // M] * M + TH[a % M][b % M]


def main():
    t0 = time.time()
    lines = []
    for a in range(T):
        for b in range(a + 1, T):
            c = third(a, b)
            if c > b:
                lines.append((a, b, c))
    print('lines', len(lines), 'build', round(time.time() - t0, 1), flush=True)
    model = cp_model.CpModel()
    x = [model.NewBoolVar(f'x{i}') for i in range(T)]
    for idx, (a, b, c) in enumerate(lines):
        model.Add(x[a] + x[b] + x[c] <= 2)
    try:
        with open('/workspace/p005/baseline.json') as f:
            pts = json.load(f)['points']
        for pt in pts:
            idx = 0
            for c in pt:
                idx = idx * 3 + c
            model.AddHint(x[idx], 1)
        print('hint added', len(pts), flush=True)
    except Exception as e:
        print('hint failed', e, flush=True)
    model.Maximize(sum(x))
    print('model ready', round(time.time() - t0, 1), flush=True)
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = 21600
    solver.parameters.num_search_workers = 64
    st = solver.Solve(model)
    obj = solver.ObjectiveValue() if st in (cp_model.OPTIMAL, cp_model.FEASIBLE) else -1
    print('status', solver.StatusName(st), 'obj', obj, 'wall', solver.WallTime(), flush=True)
    if st in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        pts = []
        for i in range(T):
            if solver.Value(x[i]):
                v = i
                c = [0] * D
                for k in range(D - 1, -1, -1):
                    c[k] = v % 3
                    v //= 3
                pts.append(c)
        with open('/workspace/p005/results/cert_global.json', 'w') as f:
            json.dump({'points': pts}, f)
        print('SAVED', len(pts), flush=True)
        if len(pts) >= 513:
            open('/workspace/p005/results/found.done', 'w').write(str(len(pts)))


if __name__ == '__main__':
    main()