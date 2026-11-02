import json
import random
import sys
import time

from ortools.sat.python import cp_model

sys.path.insert(0, '/workspace/few-shots-work/p005')
import layers  # noqa: E402

D7 = 3 ** 7
BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'


def build_lines():
    lines = []
    for a in range(D7):
        for b in range(a + 1, D7):
            c = layers.third7(a, b)
            if c > b:
                lines.append((a, b, c))
    return lines


def solve(A0, B, C, lines, time_limit=1800, workers=64):
    model = cp_model.CpModel()
    y = [model.NewBoolVar(f'y{b}') for b in range(D7)]
    z = [model.NewBoolVar(f'z{b}') for b in range(D7)]
    for (a, b, c) in lines:
        model.Add(y[a] + y[b] + y[c] <= 2)
        model.Add(z[a] + z[b] + z[c] <= 2)
    for a in A0:
        for b in range(D7):
            model.Add(y[b] + z[layers.third7(a, b)] <= 1)
    model.Maximize(sum(y) + sum(z))
    for b in B:
        model.AddHint(y[b], 1)
    for b in C:
        model.AddHint(z[b], 1)
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = time_limit
    solver.parameters.num_search_workers = workers
    st = solver.Solve(model)
    obj = solver.ObjectiveValue() if st in (cp_model.OPTIMAL, cp_model.FEASIBLE) else -1
    print('status', solver.StatusName(st), 'obj', obj, 'wall', round(solver.WallTime(), 1), flush=True)
    if st in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        NY = {b for b in range(D7) if solver.Value(y[b])}
        NZ = {b for b in range(D7) if solver.Value(z[b])}
        return NY, NZ
    return None, None


def main():
    keep = int(sys.argv[1]) if len(sys.argv) > 1 else 171
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    time_limit = int(sys.argv[3]) if len(sys.argv) > 3 else 1800
    workers = int(sys.argv[4]) if len(sys.argv) > 4 else 64
    with open(BASE) as f:
        pts = json.load(f)['points']
    L = layers.load()
    rng = random.Random(seed)
    A0 = set(rng.sample(sorted(L[0]), keep))
    print('layer0 kept', len(A0), 'of', len(L[0]), flush=True)
    lines = build_lines()
    NY, NZ = solve(A0, L[1], L[2], lines, time_limit, workers)
    if NY is None:
        return
    total = len(A0) + len(NY) + len(NZ)
    print('total', total, 'sizes', len(A0), len(NY), len(NZ), flush=True)
    if total > 512:
        out = []
        for b in A0:
            out.append([0] + layers.decode7(b))
        for b in NY:
            out.append([1] + layers.decode7(b))
        for b in NZ:
            out.append([2] + layers.decode7(b))
        with open('/workspace/p005/results/cert_layer21.json', 'w') as f:
            json.dump({'points': out}, f)
        open('/workspace/p005/results/found.done', 'w').write(str(total))
        print('SAVED', total, flush=True)


if __name__ == '__main__':
    main()