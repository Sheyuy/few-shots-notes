import json, sys
from collections import Counter
from ortools.sat.python import cp_model
sys.path.insert(0, '/workspace/few-shots-work/p005')
import capset as cs

D = int(sys.argv[1]) if len(sys.argv) > 1 else 13
TL = int(sys.argv[2]) if len(sys.argv) > 2 else 600
W = int(sys.argv[3]) if len(sys.argv) > 3 else 8
BASE = '/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json'
S = cs.load_points(BASE)
cnt = Counter()
L = sorted(S)
for i, a in enumerate(L):
    for b in L[i + 1:]:
        cnt[cs.third(a, b)] += 1
pool = set(S) | {x for x in cnt if x not in S and cnt[x] <= D}
print('d', D, 'pool', len(pool), flush=True)
lines = []
M = sorted(pool)
ps = pool
for i, a in enumerate(M):
    for b in M[i + 1:]:
        c = cs.third(a, b)
        if c > b and c in ps:
            lines.append((a, b, c))
print('lines', len(lines), flush=True)
model = cp_model.CpModel()
idx = {p: k for k, p in enumerate(M)}
x = [model.NewBoolVar(f'x{k}') for k in range(len(M))]
for a, b, c in lines:
    model.Add(x[idx[a]] + x[idx[b]] + x[idx[c]] <= 2)
model.Maximize(sum(x))
solver = cp_model.CpSolver()
solver.parameters.max_time_in_seconds = TL
solver.parameters.num_search_workers = W
st = solver.Solve(model)
obj = solver.ObjectiveValue() if st in (cp_model.OPTIMAL, cp_model.FEASIBLE) else -1
print('status', solver.StatusName(st), 'obj', obj, 'wall', round(solver.WallTime(), 1), flush=True)
if st in (cp_model.OPTIMAL, cp_model.FEASIBLE):
    chosen = {M[k] for k in range(len(M)) if solver.Value(x[k])}
    print('chosen', len(chosen), 'baseline kept', len(chosen & S), flush=True)
    if len(chosen) >= 513:
        cs.save_points('/workspace/few-shots-work/p005/shell_best.json', chosen)
        print('SAVED_513', flush=True)
