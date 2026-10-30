#!/usr/bin/env python3
"""Exact maximum-cap MILP on an archived pool, with a deletion-radius bound."""
import argparse
import json
import time
from pathlib import Path
import numpy as np
from scipy.optimize import milp, Bounds, LinearConstraint
from scipy.sparse import coo_array, vstack
from caplib import load, save, third, valid_cap, decode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('pool')
    ap.add_argument('seed')
    ap.add_argument('--remove', type=int, required=True)
    ap.add_argument('--minsize', type=int)
    ap.add_argument('--time', type=float, default=180)
    ap.add_argument('--prefix', required=True)
    ap.add_argument('--cuts')
    ap.add_argument('--fix-coordinate',type=int)
    ap.add_argument('--fix-value',type=int)
    args = ap.parse_args()
    pool = sorted(set(load(args.pool)))
    seed = load(args.seed)
    minsize = len(seed) if args.minsize is None else args.minsize
    m = len(pool)
    idx = {x: i for i, x in enumerate(pool)}
    triples = []
    t0 = time.time()
    for i, x in enumerate(pool):
        for j in range(i+1, m):
            k = idx.get(third(x, pool[j]))
            if k is not None and k > j:
                triples.append((i, j, k))
    rows, cols = [], []
    for r, tri in enumerate(triples):
        rows.extend([r]*3)
        cols.extend(tri)
    A = coo_array((np.ones(len(rows)), (rows, cols)),
                  shape=(len(triples), m)).tocsc()
    try:
        seed_cols = [idx[x] for x in seed]
    except KeyError as e:
        raise SystemExit(f'seed point outside pool: {e}')
    ncuts=0
    if args.cuts:
        cr,cc=[],[]; cut_ub=[]
        for line in Path(args.cuts).read_text().splitlines():
            if not line or line.startswith('#'): continue
            z=list(map(int,line.split()))
            if len(z)==z[0]+1:  # legacy: n followed by n pool indices; affine plane
                bound,n,vs=4,z[0],z[1:]
            elif len(z)==z[1]+2:  # explicit: bound, n, then n pool indices
                bound,n,vs=z[0],z[1],z[2:]
            else:
                raise ValueError(f'malformed cut line: {line[:100]}')
            if len(vs)!=n: raise ValueError('cut arity mismatch')
            for v in vs: cr.append(ncuts);cc.append(v)
            cut_ub.append(bound);ncuts+=1
        AC=coo_array((np.ones(len(cr)),(cr,cc)),shape=(ncuts,m)).tocsc()
    else:
        AC=coo_array((0,m)).tocsc();cut_ub=[]
    E = coo_array((np.ones(m+len(seed_cols)),
                   ([0]*m+[1]*len(seed_cols), list(range(m))+seed_cols)),
                  shape=(2, m)).tocsc()
    AA = vstack([A, AC, E], format='csc')
    lb = np.concatenate([np.full(len(triples), -np.inf),np.full(ncuts,-np.inf),
                         [minsize, len(seed)-args.remove]])
    ub = np.concatenate([np.full(len(triples), 2),np.array(cut_ub,dtype=float), [np.inf, np.inf]])
    print('BUILT', m, 'lines', len(triples), 'plane_cuts', ncuts, 'seconds', time.time()-t0, flush=True)
    vlb=np.zeros(m);vub=np.ones(m)
    if args.fix_coordinate is not None:
        if args.fix_value not in (0,1,2): raise SystemExit('--fix-value required')
        seedset=set(seed)
        for i,x in enumerate(pool):
            if decode(x)[args.fix_coordinate]==args.fix_value:
                if x in seedset: vlb[i]=1
                else: vub[i]=0
        print('FIX_LAYER',args.fix_coordinate,args.fix_value,
              'fixed_selected',int(vlb.sum()),'fixed_out',int((1-vub).sum()),flush=True)
    res = milp(c=-np.ones(m), integrality=np.ones(m), bounds=Bounds(vlb, vub),
               constraints=LinearConstraint(AA, lb, ub),
               options={'time_limit': args.time, 'mip_rel_gap': 0})
    C = [pool[i] for i, v in enumerate(res.x) if v > .5] if res.x is not None else []
    print('RESULT status', res.status, 'message', res.message,
          'objective', -res.fun if res.fun is not None else None,
          'dual_bound', -res.mip_dual_bound if res.fun is not None else None,
          'n', len(C), 'valid', valid_cap(C),
          'retained', len(set(C) & set(seed)),
          'seconds', time.time()-t0, flush=True)
    Path(args.prefix).parent.mkdir(parents=True, exist_ok=True)
    if C:
        save(args.prefix + '-model.txt', C, f'size {len(C)}')
    if len(C) >= 513 and valid_cap(C):
        out = Path(args.prefix + '-scratch')
        out.mkdir(exist_ok=True)
        pts = [decode(x) for x in C]
        (out/'certificate.json').write_text(json.dumps({'points': pts}, separators=(',', ':')))
        print('BREAKTHROUGH', len(C), flush=True)


if __name__ == '__main__':
    main()
