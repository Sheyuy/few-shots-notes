import itertools, sys, time, json
from pysat.solvers import Cadical195

n = 36
var = {}
k = 0
for i in range(n):
    for j in range(i + 1, n):
        k += 1
        var[(i, j)] = k
print("vars", k, flush=True)

clauses = []
# no K4: for every 4-set, at least one pair is a non-edge
for comb in itertools.combinations(range(n), 4):
    clauses.append([-var[(a, b)] for a, b in itertools.combinations(comb, 2)])
print("K4 clauses", len(clauses), flush=True)
# no I6: for every 6-set, at least one pair is an edge
for comb in itertools.combinations(range(n), 6):
    clauses.append([var[(a, b)] for a, b in itertools.combinations(comb, 2)])
print("total clauses", len(clauses), flush=True)

t0 = time.time()
solver = Cadical195(bootstrap_with=clauses)
print("bootstrapped in %.1fs" % (time.time() - t0), flush=True)
del clauses
t0 = time.time()
res = solver.solve()
print("solve", res, "in %.1fs" % (time.time() - t0), flush=True)
if res:
    model = set(l for l in solver.get_model() if l > 0)
    rows = []
    for i in range(n):
        row = []
        for j in range(n):
            if i == j:
                row.append('0')
            else:
                a, b = (i, j) if i < j else (j, i)
                row.append('1' if var[(a, b)] in model else '0')
        rows.append(''.join(row))
    cert = {"adjacency": rows}
    json.dump(cert, open("sat_certificate.json", "w"))
    print("wrote sat_certificate.json", flush=True)