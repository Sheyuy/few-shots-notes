import json
import sys
import time

from ortools.sat.python import cp_model

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers

BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'
D7 = layers.T7


def build_lines():
    lines = []
    for a in range(D7):
        for b in range(a + 1, D7):
            c = layers.third7(a, b)
            if c > b:
                lines.append((a, b, c))
    return lines


def main():
    L = layers.load()
    A0 = L[0]
    A1 = L[1]
    A2 = L[2]
    print('layer sizes', len(A0), len(A1), len(A2), flush=True)
    lines = build_lines()
    print('7d lines', len(lines), flush=True)

    t = time.time()
    model = cp_model.CpModel()
    y = [model.NewBoolVar(f'y{b}') for b in range(D7)]
    z = [model.NewBoolVar(f'z{b}') for b in range(D7)]
    print('vars created', round(time.time() - t, 1), flush=True)

    for i, (a, b, c) in enumerate(lines):
        model.Add(y[a] + y[b] + y[c] <= 2)
        model.Add(z[a] + z[b] + z[c] <= 2)
        if i % 200000 == 0 and i:
            print('  lines', i, round(time.time() - t, 1), flush=True)
    print('inner constraints done', round(time.time() - t, 1), flush=True)

    cnt = 0
    for a in A0:
        for b in range(D7):
            c = layers.third7(a, b)
            model.Add(y[b] + z[c] <= 1)
            cnt += 1
    print('cross constraints', cnt, round(time.time() - t, 1), flush=True)

    model.Maximize(sum(y) + sum(z))
    for b in A1:
        model.AddHint(y[b], 1)
    for b in A2:
        model.AddHint(z[b], 1)

    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = 600
    solver.parameters.num_search_workers = 2
    print('solving', flush=True)
    status = solver.Solve(model)
    print('status', solver.StatusName(status), 'objective', solver.ObjectiveValue(), flush=True)
    print('wall', solver.WallTime(), flush=True)
    if status in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        NY = {b for b in range(D7) if solver.Value(y[b])}
        NZ = {b for b in range(D7) if solver.Value(z[b])}
        print('new sizes', len(NY), len(NZ), 'total', len(A0) + len(NY) + len(NZ), flush=True)
        pts = []
        for b in A0:
            pts.append([0] + layers.decode7(b))
        for b in NY:
            pts.append([1] + layers.decode7(b))
        for b in NZ:
            pts.append([2] + layers.decode7(b))
        with open('/workspace/few-shots-work/p005/cpsat_candidate.json', 'w') as f:
            json.dump({'points': pts}, f)


if __name__ == '__main__':
    main()