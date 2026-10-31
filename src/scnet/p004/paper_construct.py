"""Rebuild the published 134,753-point construction for C7^10 exactly.

The construction is  C = B x B  plus two strips per pair:

    B  = R367 minus the 8 points r_j
    X  = tr(R367), with the single point at index 267 replaced by (1,5,6,3,5)
    J0 = {0,5,6},  J1 = {1,2,3,4,7}        <- pair 7 must be in J1

    PH = [pairs[j][0] for j in J0] + [pairs[j][1] for j in J1]
    PV = [pairs[j][1] for j in J0] + [pairs[j][0] for j in J1]
    A  = {x in X : x conflicts with some point of PV}
    D  = {x in X : x conflicts with some point of PH}

    for j in J0:  H = (x, r_j) for x in A,  V = (q_j, x) for x in D
    for j in J1:  H = (x, q_j) for x in D,  V = (r_j, x) for x in A

Size = |B|^2 + 2 * 8 * |X| = 359^2 + 16*367 = 134,753.

Omitting pair 7 from J1 (an error made once in this project) yields a set that
is 176 points short and is NOT independent; the self-check catches that.

Usage:
    python3 paper_construct.py
    python3 paper_construct.py --compare /path/to/CC_10_7_134753.txt
"""
import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path

import model as m

PATCH = (1, 5, 6, 3, 5)


def near_int(a, b):
    """Closed-neighbourhood test on encoded five-vectors."""
    return bool((m.N[a] >> b) & 1)


def build(J0=(0, 5, 6), pairs=None, patch=True):
    pairs = [tuple(tuple(p) for p in pr) for pr in (m.PAIRS8 if pairs is None else pairs)]
    pairs_i = [[m.enc5(p) for p in pr] for pr in pairs]
    d = len(pairs_i)
    X = [m.enc5(m.tr(w)) for w in m.Rlist]
    if patch:
        X[267] = m.enc5(PATCH)
    deleted = {pr[0] for pr in pairs_i}
    B = [m.enc5(w) for w in m.Rlist if m.enc5(w) not in deleted]
    J0 = set(J0)
    J1 = [j for j in range(d) if j not in J0]
    PH = [pairs_i[j][0] for j in sorted(J0)] + [pairs_i[j][1] for j in J1]
    PV = [pairs_i[j][1] for j in sorted(J0)] + [pairs_i[j][0] for j in J1]
    A = {x for x in X if any(near_int(x, p) for p in PV)}
    D = {x for x in X if any(near_int(x, p) for p in PH)}
    C = {a * m.HALF + b for a in B for b in B}
    for x in X:
        for j, pair in enumerate(pairs_i):
            inJ0 = j in J0
            ih = (inJ0 and x in A) or ((not inJ0) and x in D)
            iv = (inJ0 and x in D) or ((not inJ0) and x in A)
            C.add(x * m.HALF + pair[int(ih)])
            C.add(pair[int(iv)] * m.HALF + x)
    return C, B, A, D


def official_verifier():
    repo = Path(os.environ.get('FEW_SHOTS_REPO', '/Users/shiny/.sclaw/workspace/few-shots'))
    vp = repo / 'problems' / 'p004-shannon-capacity-c7' / 'verify' / 'verify.py'
    spec = importlib.util.spec_from_file_location('p004_verifier', vp)
    verifier = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(verifier)
    return verifier


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--compare', help='reference file, one 10-digit point per line')
    a = ap.parse_args()

    C, B, A, D = build()
    print('SIZE', len(C), 'B', len(B), 'A', len(A), 'D', len(D), flush=True)
    assert len(C) == 134753, 'construction size mismatch'
    assert m.is_valid(C), 'constructed set is not independent'

    pts = [''.join(map(str, m.DIGITS[x // m.HALF] + m.DIGITS[x % m.HALF])) for x in sorted(C)]
    verifier = official_verifier()
    verdict = verifier.verify_bytes(json.dumps({'points': pts}).encode(), verifier.load_config())
    print('OFFICIAL_VERIFY', json.dumps(verdict), flush=True)
    assert verdict['valid'], 'official verifier rejected the construction'
    assert int(verdict['score']) == 134753

    if a.compare:
        ref = {''.join(l.split()) for l in open(a.compare) if l.split()}
        mine = set(pts)
        print('COMPARE', json.dumps({'reference': len(ref), 'mine': len(mine),
                                     'only_reference': len(ref - mine),
                                     'only_mine': len(mine - ref)}), flush=True)
        if ref != mine:
            return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
