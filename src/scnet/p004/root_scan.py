"""Probe every "9th pair" candidate of `cand9.py` against the asymmetric model.

For a candidate `(r9, q9)` the construction replaces `r9` by `q9` in R and solves
the **root class only** (`root_holes=None`, so no prior optimum is assumed).
Most candidates never reach the solver: the replacement destroys the nine
private pairs, or the auxiliary sets stop being independent.

Result on 2026-11-01: 50/50 rejected — 39 "unexpected pair count",
10 "both auxiliary sets must be independent", 1 "baseR must be independent".
That is why the 9-pair family has only the R1/R2 variants in `sweep.py`.

Needs OR-Tools and FEW_SHOTS_REPO (the sweep module loads the official verifier).
"""
import argparse
import contextlib
import io
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--seconds', type=float, default=40, help='CP-SAT budget per candidate')
    ap.add_argument('--limit', type=int, default=0, help='only the first N candidates')
    ap.add_argument('--json', help='write the per-candidate records here')
    a = ap.parse_args()

    with contextlib.redirect_stdout(io.StringIO()):
        import cand9
    cands = cand9.gen_candidates_for_9th()
    if a.limit:
        cands = cands[:a.limit]
    print('CANDIDATES', len(cands), flush=True)

    import sweep
    out = []
    t0 = time.time()
    for i, (r9, q9) in enumerate(cands):
        sweep.ASYM_CHECKPOINT = None
        try:
            ck = sweep.asymmetric_patch_search(seconds=a.seconds, max_new=1, cp_seconds=5,
                                               rep_old=tuple(r9), rep_new=tuple(q9),
                                               root_holes=None)
        except (RuntimeError, ValueError) as exc:
            rec = {'i': i, 'r9': list(r9), 'q9': list(q9),
                   'status': 'STRUCTURE_REJECTED', 'error': str(exc)}
            print('ROOT', i, r9, q9, 'STRUCTURE_REJECTED', exc,
                  round(time.time() - t0, 1), flush=True)
            out.append(rec)
            continue
        h = ck['history'][0]
        rec = {'i': i, 'r9': list(r9), 'q9': list(q9), 'status': h['status'],
               'holes': h['holes'], 'lb': h['lower_bound']}
        print('ROOT', i, r9, q9, h['status'], h['holes'], h['lower_bound'],
              round(time.time() - t0, 1), flush=True)
        out.append(rec)
        if h['holes'] is not None and h['holes'] <= 16:
            print('POTENTIAL_16', json.dumps(rec), flush=True)
    if a.json:
        with open(a.json, 'w') as f:
            json.dump(out, f)


if __name__ == '__main__':
    main()
