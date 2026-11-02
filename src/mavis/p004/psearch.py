#!/usr/bin/env python3
"""psearch.py -- search for a C7^10 construction bigger than the 134753 record.

Generalises the construction of arXiv:2607.21517 (Itty et al.) to other base
sets, affine maps, partitions of the private pairs, and patches.  With the
Polak-Schrijver set R (8 private pairs) it reproduces the published 134753
record exactly; with the 367-point set P (9 private pairs, extracted from the
record set itself) it would give 358^2 + 2*9*367 = 134770.

Validity is checked exactly with the structure of the construction:
  (a) for each x, the h-values and the v-values are internally non-adjacent;
  (b) for all x,y: not( y adjacent to some h-value of x and x adjacent to some
      v-value of y ).
The B x B part is always fine (B is independent, the pool does not touch B).
"""

import itertools, json, os, sys, collections

def d5(s): return tuple(int(c) for c in s)
def adj(a, b): return all((a[i] - b[i]) % 7 in (0, 1, 6) for i in range(5))

def load_words(path):
    out = []
    for line in open(path):
        w = line.strip()
        if len(w) == 5 and all(c in "0123456" for c in w):
            out.append(w)
    return out

def private_pairs(S):
    """the private points of S (vertices outside S adjacent to exactly one point
    of S) and their partners"""
    Sd = {w: d5(w) for w in S}
    nbrs = {}
    for w in S:
        qd = Sd[w]
        for shifts in itertools.product((-1, 0, 1), repeat=5):
            cand = [(qd[k] + shifts[k]) % 7 for k in range(5)]
            cw = "".join(str(x) for x in cand)
            if cw in Sd: continue
            nbrs.setdefault(cw, set()).add(w)
    return sorted((next(iter(v)), k) for k, v in nbrs.items() if len(v) == 1)

def affine(w, spec):
    """spec: list of (sign, src, off) per output coordinate"""
    d = d5(w)
    out = []
    for sgn, src, off in spec:
        out.append((sgn * d[src] + off) % 7)
    return "".join(str(x) for x in out)

def map_spec(s):
    """'-1:2,3,0,-2:2,4' -> [(−1,1,2),(1,3,0),(1,0,0),(−1,2,2),(1,4,0)]"""
    spec = []
    for part in s.split(","):
        neg = part.startswith("-")
        body = part.lstrip("-+")
        off = 0
        if ":" in body:
            body, o = body.split(":")
            off = int(o)
        spec.append((-1 if neg else 1, int(body), off))
    return spec

def build(base, pairs, spec, j0, patch_old=None, patch_new=None):
    """returns (X, hsets, vsets) or None if the transversal is malformed"""
    Sset = set(base)
    X = []
    seen = set()
    for w in base:
        x = affine(w, spec)
        if x not in seen:
            seen.add(x); X.append(x)
    if patch_old is not None:
        if patch_old not in seen: return None
        X = [patch_new if x == patch_old else x for x in X]
        if len(set(X)) != len(X): return None
    rj = [p[0] for p in pairs]; qj = [p[1] for p in pairs]
    J1 = set(range(len(pairs))) - set(j0)
    PH = set(rj[j] for j in j0) | set(qj[j] for j in J1)
    PV = set(qj[j] for j in j0) | set(rj[j] for j in J1)
    Xd = {x: d5(x) for x in X}
    inA = {x: any(adj(Xd[x], d5(y)) for y in PH) for x in X}
    inD = {x: any(adj(Xd[x], d5(y)) for y in PV) for x in X}
    hs, vs = {}, {}
    for x in X:
        h = []; v = []
        for j in range(len(pairs)):
            if (j in j0 and inD[x]) or (j in J1 and inA[x]): h.append(qj[j])
            else: h.append(rj[j])
            if (j in j0 and inA[x]) or (j in J1 and inD[x]): v.append(qj[j])
            else: v.append(rj[j])
        hs[x] = h; vs[x] = v
    return X, hs, vs, rj, qj

def check(X, hs, vs, adjPool):
    # (a) internal non-adjacency of the h-set and the v-set of each x
    for x in X:
        for key in (hs, vs):
            st = key[x]
            for a in range(len(st)):
                da = d5(st[a])
                for b in range(a + 1, len(st)):
                    if adj(da, d5(st[b])): return False
    # (b) cross condition: not( y in H(x) and x in V(y) )
    # adjPool[p] = set of X-indices adjacent to the pool point p
    idx = {x: i for i, x in enumerate(X)}
    H = []
    for x in X:
        st = hs[x]; s = set()
        for h in st:
            s |= adjPool.get(h, set())
        H.append(s)
    V = []
    for y in X:
        st = vs[y]; s = set()
        for v in st:
            s |= adjPool.get(v, set())
        V.append(s)
    for yi in range(len(X)):
        for xi in V[yi]:
            if yi in H[xi]: return False
    return True

def main():
    basefile = sys.argv[1]
    if len(sys.argv) > 2 and os.path.exists(sys.argv[2]):
        maps = [l.strip() for l in open(sys.argv[2]) if l.strip() and not l.startswith("#")]
    elif len(sys.argv) > 2:
        maps = sys.argv[2].split(";")
    else:
        maps = ["-1:2,3,0,-2:2,4"]
    base = load_words(basefile)
    pairs = private_pairs(base)
    print(f"base={basefile} |S|={len(base)} private pairs={len(pairs)}", flush=True)
    best = None
    for mspec in maps:
        spec = map_spec(mspec)
        X0 = []
        seen = set()
        for w in base:
            x = affine(w, spec)
            if x not in seen: seen.add(x); X0.append(x)
        pool = set(p[0] for p in pairs) | set(p[1] for p in pairs)
        inside = [x for x in X0 if x in pool]
        print(f"  map {mspec}: |X0|={len(X0)} pool-inside={inside}", flush=True)
        if len(inside) > 1:
            print("   (too many pool points inside; skip)", flush=True); continue
        # candidate patches: vectors not in the pool, non-adjacent to X0 minus the pool point
        patches = [(None, None)]
        if len(inside) == 1:
            p0 = inside[0]
            # forbidden = neighbours of X0 minus the patched point
            forb = set()
            for y in X0:
                if y == p0: continue
                yd = d5(y)
                for shifts in itertools.product((-1, 0, 1), repeat=5):
                    cand = [(yd[k] + shifts[k]) % 7 for k in range(5)]
                    forb.add("".join(str(x) for x in cand))
            cands = []
            for v in range(16807):
                w = "".join(str((v // 7 ** k) % 7) for k in range(4, -1, -1))
                if w in pool or w in seen or w in forb: continue
                cands.append(w)
                if len(cands) >= 6: break
            print(f"   patch candidates: {len(cands)}", flush=True)
            patches = [(p0, c) for c in cands]
        for (po, pn) in patches:
            r = build(base, pairs, spec, set(), po, pn)
            if r is None: continue
            X, hs, vs, rj, qj = r
            # for each pool point, the X-points adjacent to it
            adjPool = {}
            for p in (set(rj) | set(qj)):
                pd = d5(p); st = set()
                for i, y in enumerate(X):
                    if adj(pd, d5(y)): st.add(i)
                adjPool[p] = st
            t = len(pairs)
            # diagnostics: how do the transversal and the pool interact?
            Xd_ = {x: d5(x) for x in X}
            poolpts = [p[0] for p in pairs] + [p[1] for p in pairs]
            cnt = collections.Counter()
            for x in X:
                c = sum(1 for y in poolpts if adj(Xd_[x], d5(y)))
                cnt[c] += 1
            print(f"   |X|={len(X)}  pool-neighbour count histogram: {dict(sorted(cnt.items()))}", flush=True)
            for mask in range(1 << t):
                j0 = set(j for j in range(t) if mask >> j & 1)
                # necessary condition: the q's of each group must be internally
                # non-adjacent (they all sit in one h- or v-set)
                J1 = set(range(t)) - j0
                ok = True
                for grp in (j0, J1):
                    qq = [qj[j] for j in grp]
                    for a in range(len(qq)):
                        for b in range(a + 1, len(qq)):
                            if adj(d5(qq[a]), d5(qq[b])): ok = False; break
                        if not ok: break
                    if not ok: break
                if not ok: continue
                r2 = build(base, pairs, spec, j0, po, pn)
                if r2 is None: continue
                X, hs, vs, rj, qj = r2
                if check(X, hs, vs, adjPool):
                    nB = len(base) - len(pairs)
                    total = nB * nB + 2 * t * len(X)
                    print(f"   VALID map={mspec} j0={sorted(j0)} patch={po}->{pn} total={total}", flush=True)
                    if best is None or total > best[0]:
                        best = (total, mspec, sorted(j0), po, pn)
    print("BEST:", best, flush=True)
    if best:
        total, mspec, j0, po, pn = best
        json.dump({"total": total, "map": mspec, "j0": j0, "patch_old": po, "patch_new": pn},
                  open("psearch_best.json", "w"), indent=1)

if __name__ == "__main__":
    main()
