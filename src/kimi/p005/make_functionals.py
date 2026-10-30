#!/usr/bin/env python3
"""Generate normalized projective functionals as row-major uint8.

Orders are explicit because historical search ids depend on row order.
The archived .bin files, not a guessed regeneration order, are authoritative
for replaying the records in results/kimi/p005/section-records/.
"""
import argparse
import itertools
import numpy as np

ap=argparse.ArgumentParser()
ap.add_argument('D',type=int)
ap.add_argument('output')
ap.add_argument('--order',choices=['trailing-code','lex'],default='trailing-code')
a=ap.parse_args()
rows=[]
if a.order=='lex':
    # Lexicographic vectors whose first nonzero coordinate is 1.
    for first in range(a.D):
        for suffix in itertools.product(range(3),repeat=a.D-first-1):
            rows.append((0,)*first+(1,)+suffix)
else:
    # Increasing little-endian code whose last nonzero coordinate is 1.
    for x in range(3**a.D):
        v=[(x//3**i)%3 for i in range(a.D)]
        if next((q for q in reversed(v) if q),0)==1:
            rows.append(v)
arr=np.array(rows,dtype=np.uint8)
assert arr.shape==((3**a.D-1)//2,a.D)
arr.tofile(a.output)
print('wrote',arr.shape,a.order,a.output)
