import itertools, sys, time, json
from pysat.solvers import Cadical195, Kissat404
from pysat.card import CardEnc, EncType

n = 36
var = {}
k = 0
for i in range(n):
    for j in range(i + 1, n):
        k += 1
        var[(i, j)] = k

d = int(sys.argv[1])
solver_name = sys.argv[2] if len(sys.argv) > 2 else "cadical"
clauses = []
# WLOG N(0) = {1,...,d}
for j in range(1, n):
    clauses.append([var[(0, j)]] if j <= d else [-var[(0, j)]])
for comb in itertools.combinations(range(n), 4):
    clauses.append([-var[(a, b)] for a, b in itertools.combinations(comb, 2)])
for comb in itertools.combinations(range(n), 6):
    clauses.append([var[(a, b)] for a, b in itertools.combinations(comb, 2)])
# every vertex has degree 11..17 (valid for any (4,6)-graph on 36 vertices)
top = k
for i in range(n):
    lits=[var[(min(i,j),max(i,j))] for j in range(n) if j!=i]
    e1=CardEnc.atleast(lits=lits, bound=11, top_id=top, encoding=EncType.seqcounter)
    clauses.extend(e1.clauses); top=e1.nv
    e2=CardEnc.atmost(lits=lits, bound=17, top_id=top, encoding=EncType.seqcounter)
    clauses.extend(e2.clauses); top=e2.nv
print("d", d, "clauses", len(clauses), flush=True)
cls = {"cadical": Cadical195, "kissat": Kissat404}[solver_name]
s = cls(bootstrap_with=clauses)
t0 = time.time()
res = s.solve()
print("d", d, solver_name, "solve", res, "%.1fs" % (time.time() - t0), flush=True)
if res:
    model = set(l for l in s.get_model() if l > 0)
    rows = []
    for i in range(n):
        rows.append(''.join('1' if i != j and var[(min(i, j), max(i, j))] in model else '0' for j in range(n)))
    json.dump({"adjacency": rows}, open("deg%d_certificate.json" % d, "w"))
    print("wrote deg%d_certificate.json" % d, flush=True)