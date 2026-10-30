import itertools, json, sys, time
from pysat.solvers import Kissat404, Cadical195

# Quotient of a 36-vertex graph by a fixed involution with 18 transpositions.
# Pairs P_0..P_17, each {2i, 2i+1}.
#   x[i][j] : all four edges between P_i and P_j present (i<j)
#   y[i]    : edge inside P_i present
# The lift has no K4 and no I6 iff the clauses below hold.

m = 18
def X(i, j):
    if i > j: i, j = j, i
    return ("x", i, j)
def Y(i): return ("y", i)

clauses = []
# ---- no K4 ----
# (2,2): both pairs contribute both vertices
for i in range(m):
    for j in range(i+1, m):
        clauses.append([-1])  # placeholder replaced below
clauses = []

def lit(t):
    key, a = t[0], t[1:]
    if key == "x":
        i, j = a
        return var_x[(i, j)]
    return var_y[a[0]]

var_x = {}
k = 0
for i in range(m):
    for j in range(i+1, m):
        k += 1; var_x[(i, j)] = k
var_y = {}
for i in range(m):
    k += 1; var_y[i] = k

def LX(i, j): return var_x[(min(i, j), max(i, j))]
def LY(i): return var_y[i]

# K4 type (2,2)
for i in range(m):
    for j in range(i+1, m):
        clauses.append([-LX(i, j), -LY(i), -LY(j)])
# K4 type (2,1,1)
for i in range(m):
    for j in range(m):
        for kk in range(j+1, m):
            if j == i or kk == i: continue
            clauses.append([-LY(i), -LX(i, j), -LX(i, kk), -LX(j, kk)])
# K4 type (1,1,1,1)
for i, j, kk, l in itertools.combinations(range(m), 4):
    clauses.append([-LX(i, j), -LX(i, kk), -LX(i, l), -LX(j, kk), -LX(j, l), -LX(kk, l)])
print("K4 clauses", len(clauses), flush=True)

# ---- no I6 ----
# (2,2,2)
for i, j, kk in itertools.combinations(range(m), 3):
    clauses.append([LX(i, j), LX(i, kk), LX(j, kk), LY(i), LY(j), LY(kk)])
# (2,2,1,1): pairs i,j double, pairs kk,l single
for i, j, kk, l in itertools.combinations(range(m), 4):
    # which two pairs are doubled: any choice - clause is symmetric in the four pairs
    # if i,j doubled and kk,l single, forbidden iff all cross absent and y_i,y_j absent
    # we can add all 6 choices, or just note symmetry: clause must include y for the doubled ones only.
    pass
# add explicitly for each choice of doubled pair among 4
for quad in itertools.combinations(range(m), 4):
    a, b, c, d = quad
    pairs = [(a, b), (a, c), (a, d), (b, c), (b, d), (c, d)]
    for di in range(4):
        for dj in range(di+1, 4):
            doubled = (quad[di], quad[dj])
            cl = [LX(u, v) for u, v in pairs]
            cl.append(LY(doubled[0])); cl.append(LY(doubled[1]))
            clauses.append(cl)
# (2,1,1,1,1)
for five in itertools.combinations(range(m), 5):
    a, b, c, d, e = five
    pairs = list(itertools.combinations(five, 2))
    for di in range(5):
        cl = [LX(u, v) for u, v in pairs]
        cl.append(LY(five[di]))
        clauses.append(cl)
# (1,1,1,1,1,1)
for six in itertools.combinations(range(m), 6):
    clauses.append([LX(u, v) for u, v in itertools.combinations(six, 2)])
print("total clauses", len(clauses), flush=True)

name = sys.argv[1] if len(sys.argv) > 1 else "kissat"
if name == "kissat":
    s = Kissat404(bootstrap_with=clauses)
else:
    s = Cadical195(bootstrap_with=clauses)
t0 = time.time()
res = s.solve()
print("solve", res, "%.2fs" % (time.time() - t0), flush=True)
if res:
    model = set(l for l in s.get_model() if l > 0)
    rows = []
    for v in range(36):
        i, bi = divmod(v, 2)
        row = []
        for w in range(36):
            if v == w: row.append('0'); continue
            j, bj = divmod(w, 2)
            if i == j:
                row.append('1' if LY(i) in model else '0')
            else:
                row.append('1' if LX(i, j) in model else '0')
        rows.append(''.join(row))
    json.dump({"adjacency": rows}, open("invq_certificate.json", "w"))
    print("wrote invq_certificate.json", flush=True)