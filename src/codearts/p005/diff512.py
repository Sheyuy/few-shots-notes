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


def enc(pt):
    v = 0
    for c in pt:
        v = v * 3 + c
    return v


def main():
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
    tl = int(sys.argv[2]) if len(sys.argv) > 2 else 3600
    w = int(sys.argv[3]) if len(sys.argv) > 3 else 64
    base_path = sys.argv[4] if len(sys.argv) > 4 else '/workspace/p005/baseline.json'
    out_path = sys.argv[5] if len(sys.argv) > 5 else f'/workspace/p005/results/cert_diff_{seed}.json'
    lines = []
    for a in range(T):
        for b in range(a + 1, T):
            c = third(a, b)
            if c > b:
                lines.append((a, b, c))
    print('lines', len(lines), flush=True)
    base = json.load(open(base_path))['points']
    hinted = {enc(p) for p in base}
    model = cp_model.CpModel()
    x = [model.NewBoolVar(f'x{i}') for i in range(T)]
    for a, b, c in lines:
        model.Add(x[a] + x[b] + x[c] <= 2)
    for i in hinted:
        model.AddHint(x[i], 1)
    lits = [x[i].Not() if i in hinted else x[i] for i in range(T)]
    model.AddBoolOr(lits)
    model.Maximize(sum(x))
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = tl
    solver.parameters.num_search_workers = w
    solver.parameters.random_seed = seed
    st = solver.Solve(model)
    obj = solver.ObjectiveValue() if st in (cp_model.OPTIMAL, cp_model.FEASIBLE) else -1
    print('status', solver.StatusName(st), 'obj', obj, 'wall', round(solver.WallTime(), 1), flush=True)
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
        json.dump({'points': pts}, open(out_path, 'w'))
        print('SAVED', len(pts), out_path, flush=True)
        if len(pts) >= 513:
            open('/workspace/p005/results/found.done', 'w').write(str(len(pts)))


if __name__ == '__main__':
    main()