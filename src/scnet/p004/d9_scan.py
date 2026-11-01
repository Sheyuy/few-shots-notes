"""Scan the 9th pair of the paper construction (d=9 -> 134,770).

For every candidate (r9, q9) from cand9.gen_candidates_for_9th() and every
3-subset J0 of the 9 pair indices, build the point set with
paper_construct.build(pairs=PAIRS8+[(r9,q9)], J0=J0) and test independence.

Result 2026-11-01: 1,450 constructions, every one has exactly 134,770 points,
none is independent. The first conflict is always inside a single row
(two suffixes of the same prefix conflict).
"""
import sys, os, json, itertools, contextlib, io, time
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import model as m
import paper_construct as pc

with contextlib.redirect_stdout(io.StringIO()):
    import cand9
cands = cand9.gen_candidates_for_9th()

def first_conflict(C):
    groups = {}
    for t in C:
        p, s = divmod(t, m.HALF)
        groups.setdefault(p, []).append(s)
    members = {}; reach = {}
    for p, ss in groups.items():
        mm = u = 0
        for s in ss:
            if (u >> s) & 1: return ('row', p, s)
            mm |= 1 << s; u |= m.N[s]
        members[p] = mm; reach[p] = u
    for p, u in reach.items():
        for q in m.bits(m.N[p]):
            if q > p and members.get(q, 0) & u:
                return ('cross', p, q)
    return None

J0S = [(0,5,6)] + [tuple(sorted((a,b,8))) for a,b in itertools.combinations(range(8),2)]
tested = size_ok = valid = 0
kinds = {}
t0 = time.time()
for i, (r9, q9) in enumerate(cands):
    pairs = list(m.PAIRS8) + [(tuple(r9), tuple(q9))]
    row = {'i': i, 'r9': list(r9), 'q9': list(q9), 'builds': 0, 'size_ok': 0, 'valid': 0}
    for J0 in J0S:
        C, B, A, D = pc.build(pairs=pairs, J0=J0)
        tested += 1; row['builds'] += 1
        if len(C) == 134770:
            size_ok += 1; row['size_ok'] += 1
            fc = first_conflict(C)
            if fc is None: valid += 1; row['valid'] += 1
            else: kinds[fc[0]] = kinds.get(fc[0], 0) + 1
    print('CAND', json.dumps(row), flush=True)
print('SUMMARY candidates=%d tested=%d size_134770=%d valid=%d conflicts=%s seconds=%.1f'
      % (len(cands), tested, size_ok, valid, json.dumps(kinds), time.time()-t0), flush=True)
