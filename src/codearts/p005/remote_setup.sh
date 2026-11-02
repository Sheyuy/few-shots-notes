#!/bin/bash
set -e
export DEBIAN_FRONTEND=noninteractive
mkdir -p /root/pylibs /root/p005
PIP=/root/pylibs/bin/pip
if [ ! -x "$PIP" ]; then
  curl -sSL --max-time 300 https://bootstrap.pypa.io/get-pip.py -o /tmp/get-pip.py
  /usr/bin/python3.10 /tmp/get-pip.py --target /root/pylibs >/tmp/getpip.log 2>&1 || true
fi
if [ ! -x "$PIP" ]; then
  URL=$(curl -sSL https://pypi.org/pypi/pip/json | /usr/bin/python3.10 -c "import sys,json;d=json.load(sys.stdin);print([u['url'] for u in d['urls'] if u['filename'].endswith('.whl')][0])")
  curl -sSL --max-time 300 -o /tmp/pip.whl "$URL"
  /usr/bin/python3.10 -m zipfile -e /tmp/pip.whl /root/pylibs
fi
export PYTHONPATH=/root/pylibs
/usr/bin/python3.10 -m pip --version
/usr/bin/python3.10 -m pip install --quiet --target /root/pylibs ortools
/usr/bin/python3.10 -c "from ortools.sat.python import cp_model; print('ortools ready')"
cat > /root/p005/global_cpsat.py <<'PY'
import json, sys, time
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
            a //= 3; b //= 3; p *= 3
        TH[u][v] = w
def third(a, b):
    return TH[a // M][b // M] * M + TH[a % M][b % M]
lines = []
for a in range(T):
    for b in range(a + 1, T):
        c = third(a, b)
        if c > b:
            lines.append((a, b, c))
print('lines', len(lines), flush=True)
model = cp_model.CpModel()
x = [model.NewBoolVar(f'x{i}') for i in range(T)]
for idx, (a, b, c) in enumerate(lines):
    model.Add(x[a] + x[b] + x[c] <= 2)
model.Maximize(sum(x))
solver = cp_model.CpSolver()
solver.parameters.max_time_in_seconds = 7200
solver.parameters.num_search_workers = 256
st = solver.Solve(model)
print('status', solver.StatusName(st), 'obj', solver.ObjectiveValue(), 'wall', solver.WallTime(), flush=True)
if st in (cp_model.OPTIMAL, cp_model.FEASIBLE) and solver.ObjectiveValue() >= 513:
    pts = []
    for i in range(T):
        if solver.Value(x[i]):
            v = i
            c = [0] * D
            for k in range(D - 1, -1, -1):
                c[k] = v % 3; v //= 3
            pts.append(c)
    json.dump({'points': pts}, open('/root/p005/certificate.json', 'w'))
    print('SAVED', len(pts), flush=True)
PY
nohup env PYTHONPATH=/root/pylibs /usr/bin/python3.10 /root/p005/global_cpsat.py > /root/p005/global.log 2>&1 &
echo "started pid $!"