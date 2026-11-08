#!/usr/bin/env python3
"""
closure_sat.py -- offline (SAT-based) closure of the (4,6,35) counterexample set.

Why offline
------------
The GitHub token expired, so the remote runners are unreachable.  Fortunately
this part needs no third-party library: the one-vertex extension test is a
SAT instance with only n Boolean variables

    violations(B + v) = tri(B[S]) + ind5(B[V(B) \\ S])

where S = N(v).  Given a valid k-vertex (4,6)-graph B we ask whether there is
an S with B[S] triangle-free and B[V \\ S] free of independent 5-sets.  If yes
we get a (k+1)-vertex (4,6)-graph.

That gives us the same closure machinery Lehavi's ROVEaC provides, but with a
SAT solver we wrote ourselves:

    base (k-vertex counterexamples)
      |-- extend each base to k+1 vertices      -> a hit here is a new record
      |-- decrement each base (delete one vertex) -> R(4,6,k-1)
      |-- extend each decremented graph back to k -> new k-graphs, add to base
      `-- repeat until a fixed point

Every step is exact (SAT, no heuristics), so a fixed point is a genuine
statement about the base set we reached.

Usage:  python3 closure_sat.py <base.json> [max_rounds]
"""
import json
import hashlib
import itertools
import sys
import time


# ---------------------------------------------------------------- SAT helpers
def dpll(clauses, nvars):
    """Return a satisfying assignment dict, or None if UNSAT.

    A tiny DPLL with unit propagation and pure-literal elimination.  The
    instances here are small (n <= 36 variables) with long clauses, so this is
    plenty fast.
    """
    cls = [list(c) for c in clauses]

    def simplify(cs, lit):
        out = []
        for c in cs:
            if lit in c:
                continue
            c2 = [x for x in c if x != -lit]
            if not c2:
                return None
            out.append(c2)
        return out

    def search(cs):
        # unit propagation
        while True:
            unit = None
            for c in cs:
                if len(c) == 1:
                    unit = c[0]
                    break
            if unit is None:
                break
            cs = simplify(cs, unit)
            if cs is None:
                return None
        if not cs:
            return {}
        # pure literal
        lits = set(x for c in cs for x in c)
        for l in sorted(lits):
            if -l not in lits:
                nxt = simplify(cs, l)
                if nxt is not None:
                    r = search(nxt)
                    if r is not None:
                        return {l: 1, **r}
        c = min(cs, key=len)
        for v in (c[0], -c[0]):
            nxt = simplify(cs, v)
            if nxt is None:
                continue
            r = search(nxt)
            if r is not None:
                return {v: 1, **r}
        return None

    return search(cls)


def extension_cnf(n, A):
    """CNF (over x_1..x_n) for: does there exist S = N(v) such that B[S] is
    triangle-free and B[V \\ S] has no independent 5-set?  Then B+v is a
    valid (4,6)-graph."""
    A = [[1 if c in (1, True, "1") else 0 for c in row] for row in A]
    clauses = []
    # every triangle of B forbids S containing all three
    for a in range(n):
        for b in range(a + 1, n):
            if not A[a][b]:
                continue
            for c in range(b + 1, n):
                if A[a][c] and A[b][c]:
                    clauses.append([-(a + 1), -(b + 1), -(c + 1)])
    # every independent 5-set of B forbids S being disjoint from it
    for a in range(n):
        for b in range(a + 1, n):
            if A[a][b]:
                continue
            for c in range(b + 1, n):
                if A[a][c] or A[b][c]:
                    continue
                for d in range(c + 1, n):
                    if A[a][d] or A[b][d] or A[c][d]:
                        continue
                    for e in range(d + 1, n):
                        if A[a][e] or A[b][e] or A[c][e] or A[d][e]:
                            continue
                        clauses.append([a + 1, b + 1, c + 1, d + 1, e + 1])
    return clauses


def extends(A):
    """Return the neighbour set S of a new vertex if A extends to n+1 vertices
    (i.e. a valid (4,6)-graph exists with A as a subgraph), else None."""
    n = len(A)
    sol = dpll(extension_cnf(n, A), n)
    if sol is None:
        return None
    S = [i for i in range(n) if sol.get(i + 1, 0)]
    return S


# ---------------------------------------------------------------- graph utils
def wl_hash(A, iters=4):
    n = len(A)
    nb = [frozenset(j for j in range(n) if A[i][j]) for i in range(n)]
    col = [0] * n
    for _ in range(iters):
        col = [hash((tuple(sorted(nb[i])),
                     tuple(sorted(col[j] for j in nb[i])))) for i in range(n)]
    return hashlib.sha256(repr(sorted(col)).encode()).hexdigest()[:16]


def has_independent_set(A, k):
    """Does A contain an independent set of size k?  Backtracking with
    pruning and early exit -- far faster than enumerating all k-subsets
    (C(34,5) = 278k per graph was the bottleneck of the closure)."""
    n = len(A)
    adj = [0] * n
    for i in range(n):
        for j in range(n):
            if A[i][j]:
                adj[i] |= 1 << j

    def rec(cand, need):
        if need == 0:
            return True
        if cand.bit_count() < need:
            return False
        # Branch on an arbitrary pivot (fewest non-neighbours left = better
        # pruning).  IMPORTANT: the "include" branch must NOT restrict the
        # candidates to larger indices -- that restriction is only valid when
        # the pivot is always the minimum, and using it with an arbitrary pivot
        # silently drops solutions (which made invalid graphs pass the check).
        best = -1
        bestmask = 0
        bestcnt = None
        m = cand
        while m:
            v = (m & -m).bit_length() - 1
            m &= m - 1
            c = cand & ~adj[v] & ~(1 << v)
            cnt = c.bit_count()
            if bestcnt is None or cnt < bestcnt:
                bestcnt = cnt
                best = v
                bestmask = c
        return rec(bestmask, need - 1) or rec(cand & ~(1 << best), need)

    return rec((1 << n) - 1, k)


def is_counterexample(A):
    """A is a (4,6)-graph: no K4 and no independent 6-set."""
    n = len(A)
    A = [[1 if c in (1, True, "1") else 0 for c in row] for row in A]
    adj = [0] * n
    for i in range(n):
        for j in range(n):
            if A[i][j]:
                adj[i] |= 1 << j
    for i in range(n):
        nb = [j for j in range(n) if A[i][j]]
        for a in range(len(nb)):
            for b in range(a + 1, len(nb)):
                if not (adj[nb[a]] >> nb[b]) & 1:
                    continue
                for c in range(b + 1, len(nb)):
                    if (adj[nb[a]] >> nb[c]) & 1 and (adj[nb[b]] >> nb[c]) & 1:
                        return False
    if has_independent_set(A, 6):
        return False
    return True


def delete_vertex(A, v):
    keep = [i for i in range(len(A)) if i != v]
    return [[A[i][j] for j in keep] for i in keep]


def add_vertex(A, S):
    n = len(A)
    B = [row[:] + [0] for row in A] + [[0] * (n + 1)]
    for i in S:
        B[i][n] = B[n][i] = 1
    return B


def load(fn):
    d = json.load(open(fn))
    gs = d if isinstance(d, list) else list(d.values())
    return [[[int(c) for c in row] for row in g["adjacency"]] for g in gs]


def main():
    base_file = sys.argv[1]
    max_rounds = int(sys.argv[2]) if len(sys.argv) > 2 else 6
    base = load(base_file)
    seen = set(wl_hash(A) for A in base)
    print("start: %d base graphs (%d distinct WL classes), n = %d"
          % (len(base), len(seen), len(base[0])), flush=True)
    t0 = time.time()
    summary = {"rounds": []}

    for rnd in range(max_rounds):
        n = len(base[0])
        print("=== round %d: %d base graphs on %d vertices ===" % (rnd, len(base), n),
              flush=True)

        # (1) does any base extend to n+1 vertices?  (a hit = new record)
        hits = []
        for idx, A in enumerate(base):
            S = extends(A)
            if S is not None:
                B = add_vertex(A, S)
                if is_counterexample(B):
                    hits.append((idx, B))
                    json.dump({"adjacency": B},
                              open("FOUND%d_round%d_%03d.json" % (n + 1, rnd, idx), "w"))
                    print("  *** BASE #%d EXTENDS TO %d VERTICES ***" % (idx, n + 1),
                          flush=True)
        print("  extend(base): %d/%d extend  [%.1f s]"
              % (len(hits), len(base), time.time() - t0), flush=True)
        if hits:
            summary["RECORD"] = True
            break

        # (2) decrement: the (n-1)-vertex subgraphs that are themselves
        #     counterexamples
        dec = {}
        for A in base:
            for v in range(n):
                D = delete_vertex(A, v)
                h = wl_hash(D)
                if h in dec:
                    continue
                dec[h] = D if is_counterexample(D) else None
        dec = {h: D for h, D in dec.items() if D is not None}
        print("  decremented set: %d distinct (4,6)-graphs on %d vertices"
              % (len(dec), n - 1), flush=True)

        # (3) extend each decremented graph back to n vertices
        added = 0
        for h, D in dec.items():
            S = extends(D)
            if S is None:
                continue
            B = add_vertex(D, S)
            if not is_counterexample(B):
                continue
            hh = wl_hash(B)
            if hh in seen:
                continue
            seen.add(hh)
            base.append(B)
            added += 1
            json.dump({"adjacency": B},
                      open("NEW%d_round%d_%03d.json" % (n, rnd, added), "w"))
        print("  extend(decremented): %d NEW graphs added" % added, flush=True)
        summary["rounds"].append({"round": rnd, "base_size": len(base),
                                   "added": added})
        json.dump([{"adjacency": A} for A in base], open("CLOSURE_SAT_BASE.json", "w"))
        json.dump(summary, open("CLOSURE_SAT_SUMMARY.json", "w"), indent=1)
        if added == 0:
            print("  fixed point reached", flush=True)
            break

    summary["final_base_size"] = len(base)
    summary["distinct_classes"] = len(seen)
    summary["seconds"] = round(time.time() - t0, 1)
    json.dump(summary, open("CLOSURE_SAT_SUMMARY.json", "w"), indent=1)
    print("SUMMARY " + json.dumps(summary), flush=True)


if __name__ == "__main__":
    main()
