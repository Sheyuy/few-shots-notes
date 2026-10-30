#!/usr/bin/env python3
"""Verify a recorded projective subspace intersection without rerunning search."""
import argparse
import numpy as np

ap=argparse.ArgumentParser()
ap.add_argument('matrix')
ap.add_argument('functionals')
ap.add_argument('record')
ap.add_argument('D',type=int)
a=ap.parse_args()
rows=[s.strip() for s in open(a.matrix) if s.strip()]
assert len(rows)==a.D and len(set(map(len,rows)))==1
M=np.array([[int(c) for c in row] for row in rows],dtype=np.int16)
F=np.fromfile(a.functionals,dtype=np.uint8).reshape(-1,a.D).astype(np.int16)
first=open(a.record).readline().split()
assert first[0]=='global'
claimed=int(first[1]);h=list(map(int,first[2:]))
vals=(F[h]@M)%3
ids=np.flatnonzero(np.all(vals==0,axis=0))
print('record',a.record,'claimed',claimed,'actual',len(ids),
      'functionals',h,'PASS' if len(ids)==claimed else 'FAIL')
raise SystemExit(0 if len(ids)==claimed else 1)
