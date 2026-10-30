import json
import sys
import time

from ortools.sat.python import cp_model


def tables(d):
    h = d // 2
    m1 = 3 ** h
    m2 = 3 ** (d - h)
    t1 = [[0] * m1 for _ in range(m1)]
    for u in range(m1):
        for v in range(m1):
            w = 0
            p = 1
            a, b = u, v
            for _ in range(h):
                w += (-(a % 3 + b % 3)) % 3 * p
                a //= 3
                b //= 3
                p *= 3
            t1[u][v] = w
    t2 = [[0] * m2 for _ in range(m2)]
    for u in range(m2):
        for v in range(m2):
            w = 0
            p = 1
            a, b = u, v
            for _ in range(d - h):
                w += (-(a % 3 + b % 3)) % 3 * p
                a //= 3
                b //= 3
                p *= 3
            t2[u][v] = w
    return t2, m2, t1, m1


def make_third(d):
    t2, m2, t1, m1 = tables(d)
    def third(a, b):
        return t2[a // m1][b // m1] * m1 + t1[a % m1][b % m1]
    return third


def run(d, time_limit=600, workers=8, fixed=True, out=None):
    T = 3 ** d
    third = make_third(d)
    lines = []
    for a in range(T):
        for b in range(a + 1, T):
            c = third(a, b)
            if c > b:
                lines.append((a, b, c))
    print('d', d, 'lines', len(lines), flush=True)
    model = cp_model.CpModel()
    x = [model.NewBoolVar(f'x{i}') for i in range(T)]
    for a, b, c in lines:
        model.Add(x[a] + x[b] + x[c] <= 2)
    if fixed:
        pts = [0] + [3 ** (d - 1 - i) for i in range(d)]
        for p in pts:
            model.Add(x[p] == 1)
        print('fixed standard points', len(pts), flush=True)
    target = 0
    if len(sys.argv) > 6 and sys.argv[6].isdigit():
        target = int(sys.argv[6])
    if target:
        model.Add(sum(x) >= target)
        print('feasibility target', target, flush=True)
    else:
        model.Maximize(sum(x))
    solver = cp_model.CpSolver()
    solver.parameters.max_time_in_seconds = time_limit
    solver.parameters.num_search_workers = workers
    st = solver.Solve(model)
    obj = solver.ObjectiveValue() if st in (cp_model.OPTIMAL, cp_model.FEASIBLE) else -1
    print('status', solver.StatusName(st), 'obj', obj, 'wall', round(solver.WallTime(), 2), flush=True)
    if out and st in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        pts_out = []
        for i in range(T):
            if solver.Value(x[i]):
                v = i
                c = [0] * d
                for k in range(d - 1, -1, -1):
                    c[k] = v % 3
                    v //= 3
                pts_out.append(c)
        json.dump({'points': pts_out}, open(out, 'w'))
        print('SAVED', len(pts_out), flush=True)


if __name__ == '__main__':
    d = int(sys.argv[1]) if len(sys.argv) > 1 else 6
    tl = int(sys.argv[2]) if len(sys.argv) > 2 else 600
    w = int(sys.argv[3]) if len(sys.argv) > 3 else 8
    fix = (len(sys.argv) <= 4 or sys.argv[4] != 'nofix')
    out = sys.argv[5] if len(sys.argv) > 5 else None
    run(d, tl, w, fix, out)