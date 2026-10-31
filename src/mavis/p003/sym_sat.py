#!/usr/bin/env python3
"""Symmetry-class SAT search for a 36-vertex (4,6)-Ramsey graph.

A graph on 36 vertices is coloured with two colours (friend / not-friend).
We require: no monochromatic K4 in colour 1 and no independent set of 6
(i.e. no K6 in colour 2).  Searching the full space is hopeless, so we
restrict to graphs invariant under a group G of order 4 acting on the 36
vertices.  Then each *edge orbit* of G carries a single colour, and the
problem becomes a SAT instance with only ~150-200 variables:

  * variable x_o = 1  <=>  the edge orbit o is coloured 1
  * for every 4-subset S:  clause  OR_e NOT x_{o(e)}     (not all six edges
                            of S may be colour 1)
  * for every 6-subset S:  clause  OR_e x_{o(e)}         (not all fifteen edges
                            of S may be colour 2)

Only one representative per G-orbit of subsets is needed, which cuts the
clause count by a factor of |G|.

usage: sym_sat.py <family> [r] [--out cnf] [--solve] [--limit N]
"""
import itertools, sys, json, os

N = 36
N_OVERRIDE = None


# ---------------------------------------------------------------- groups
def group_inf35(r):
    """V4 = {1,-1,r,-r} acting on Z_35 plus a fixed point (Exoo-like)."""
    def mk(f):
        p = list(range(N))
        for i in range(35):
            p[i] = f(i) % 35
        p[35] = 35
        return tuple(p)
    return [mk(lambda x: x), mk(lambda x: -x), mk(lambda x: r * x), mk(lambda x: -r * x)]


def group_z36(c=0):
    """V4 = <x -> x+18, x -> -x+c> acting on Z_36 (c is a free parameter)."""
    def mk(f):
        return tuple(f(i) % 36 for i in range(36))
    # b = x -> -x + c, a = x -> x + 18; then b.a = x -> -x + c + 18 (mod 36)
    return [mk(lambda x: x), mk(lambda x: x + 18), mk(lambda x: -x + c),
            mk(lambda x: -x + c + 18)]


def group_zdih(m=9, c=0):
    """Generalised dihedral family on Z_36: <x -> x+m> (order 36/m) together
    with the involution x -> -x+c.  The group order is 2*(36/m):
      m=9  -> order 8   (90 edge orbits)
      m=6  -> order 12
      m=4  -> order 18
      m=3  -> order 24
      m=2  -> order 36  (the full dihedral group: ~40 edge orbits)
    Larger groups => far fewer variables => much easier SAT, but a more
    restrictive class.  Sweeping m is the systematic way through the
    symmetry tower (Exoo's and the BPZ approach)."""
    def mk(f):
        return tuple(f(i) % 36 for i in range(36))
    g = []
    for k in range(36 // m):
        for refl in (0, 1):
            if refl == 0:
                g.append(mk(lambda x, k=k, m=m: x + m * k))
            else:
                g.append(mk(lambda x, k=k, m=m, c=c: -(x + m * k) + c))
    return g


def group_zdih8(c=0):
    """Dihedral group of order 8 on Z_36: <x -> x+9> (order 4) together with
    the involution x -> -x+c.  Eight elements means far fewer edge orbits
    (~85 instead of ~171), which makes the SAT instance much easier."""
    def mk(f):
        return tuple(f(i) % 36 for i in range(36))
    g = []
    for k in range(4):
        for refl in (0, 1):
            if refl == 0:
                g.append(mk(lambda x, k=k: x + 9 * k))
            else:
                g.append(mk(lambda x, k=k, c=c: -(x + 9 * k) + c))
    return g


def group_zneg(n):
    """Order-2 group {id, x -> -x} on Z_n.  For n=8 the cycle C_8 is a
    valid (4,6)-graph invariant under this group, so the SAT instance for
    n=8 must be satisfiable and the extracted graph must verify: a
    complete end-to-end test of the CNF encoding and of extract.py."""
    def mk(f):
        return tuple(f(i) % n for i in range(n))
    return [mk(lambda x: x), mk(lambda x: -x)]


def group_z36_p9():
    """Z4 = <x -> x+9> acting on Z_36."""
    def mk(f):
        return tuple(f(i) % 36 for i in range(36))
    return [mk(lambda x: x), mk(lambda x: x + 9), mk(lambda x: x + 18), mk(lambda x: x + 27)]


FAMILIES = {
    "inf35": group_inf35,
    "z36": group_z36,
    "z36p9": group_z36_p9,
}


# ------------------------------------------------------- orbit machinery
def edge_orbits(group):
    """Partition the edges of K_36 into orbits under the group."""
    edges = list(itertools.combinations(range(N), 2))
    idx = {e: i for i, e in enumerate(edges)}
    orb = [None] * len(edges)
    nxt = 0
    for e in edges:
        if orb[idx[e]] is not None:
            continue
        images = set()
        for g in group:
            a, b = g[e[0]], g[e[1]]
            images.add((a, b) if a < b else (b, a))
        for im in images:
            orb[idx[im]] = nxt
        nxt += 1
    return {e: orb[idx[e]] for e in edges}, nxt


def mask_of(sub):
    m = 0
    for v in sub:
        m |= 1 << v
    return m


def perm_mask(g, m):
    out = 0
    mm = m
    while mm:
        v = (mm & -mm).bit_length() - 1
        mm &= mm - 1
        out |= 1 << g[v]
    return out


def subset_representatives(group, size):
    """Yield one representative per group-orbit of size-subsets."""
    seen = set()
    for sub in itertools.combinations(range(N), size):
        m = mask_of(sub)
        if m in seen:
            continue
        orbit = set()
        for g in group:
            orbit.add(perm_mask(g, m))
        seen |= orbit
        yield sub


# ------------------------------------------------------------------ CNF
def build_cnf(group, limit4=None, limit6=None):
    orb, nvar = edge_orbits(group)
    clauses = []
    # 4-subsets: not all six edges in colour 1
    for sub in subset_representatives(group, 4):
        lits = set()
        for a, b in itertools.combinations(sub, 2):
            lits.add(-(orb[idx_of(a, b)] + 1))      # NOT x_o
        clauses.append(sorted(lits))
        if limit4 and len(clauses) >= limit4:
            break
    # 6-subsets: not all fifteen edges in colour 2
    for sub in subset_representatives(group, 6):
        lits = set()
        for a, b in itertools.combinations(sub, 2):
            lits.add(orb[idx_of(a, b)] + 1)          # x_o
        clauses.append(sorted(lits))
        if limit6 and len(clauses) >= limit6:
            break
    return clauses, nvar, orb


def idx_of(a, b):
    return (a, b) if a < b else (b, a)


def write_dimacs(path, clauses, nvar):
    with open(path, "w") as f:
        f.write(f"p cnf {nvar} {len(clauses)}\n")
        for c in clauses:
            f.write(" ".join(str(x) for x in c) + " 0\n")


# ----------------------------------------------------------------- main
def main():
    fam = sys.argv[1] if len(sys.argv) > 1 else "inf35"
    r = int(sys.argv[2]) if len(sys.argv) > 2 and sys.argv[2].isdigit() else 6
    args = sys.argv[3:]
    tag = fam + (("_%d" % r) if fam == "inf35" else "")
    out = "cnf/%s.cnf" % tag
    os.makedirs("cnf", exist_ok=True)
    if fam.startswith("zneg"):
        global N
        N = int(fam[4:]) if len(fam) > 4 else 8
        group = group_zneg(N)
    elif fam.startswith("zdihG"):
        # zdihG<m>[c] : generalised dihedral with step m and reflection c
        body = fam[5:]
        mm = 9; cc = 0
        if body.startswith("m"):
            body = body[1:]
            num = ""
            for ch in body:
                if ch.isdigit(): num += ch
                else: break
            mm = int(num) if num else 9
            body = body[len(num):]
        if body.startswith("c") and body[1:].isdigit():
            cc = int(body[1:])
        group = group_zdih(mm, cc)
    elif fam.startswith("zdih8"):
        tail = fam[5:]
        cv = int(tail[1:]) if tail.startswith("c") and tail[1:].isdigit() else 0
        group = group_zdih8(cv)
    elif fam == "inf35":
        group = group_inf35(r)
    elif fam.startswith("z36c"):
        group = group_z36(int(fam[4:]) if len(fam) > 4 else 0)
    else:
        group = FAMILIES[fam]()
    print("family", fam, "r" if fam == "inf35" else "", "r=", r)
    clauses, nvar, orb = build_cnf(group)
    print("variables (edge orbits):", nvar, " clauses:", len(clauses))
    write_dimacs(out, clauses, nvar)
    print("wrote", out, os.path.getsize(out), "bytes")
    json.dump({"family": fam, "r": r, "group": [list(g) for g in group],
               "nvar": nvar,
               "orb": [orb[idx_of(a, b)] for a, b in itertools.combinations(range(N), 2)]},
              open("cnf/%s.json" % tag, "w"))
    print("wrote the orbit data")


if __name__ == "__main__":
    main()
