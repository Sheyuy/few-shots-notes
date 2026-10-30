#!/usr/bin/env python3
"""Generate Edel/Tyrrell's 236-point cap in F_3^7 and its 8D layer embedding."""
import argparse
from itertools import combinations, product
from pathlib import Path
from caplib import save

def main():
 ap=argparse.ArgumentParser();ap.add_argument('outdir');args=ap.parse_args()
 out=Path(args.outdir);out.mkdir(parents=True,exist_ok=True)
 supports={(0,1,2),(0,1,3),(0,2,4),(0,3,5),(0,4,5),
           (1,2,5),(1,3,4),(1,4,5),(2,3,4),(2,3,5)}
 D=[];Dp=[]
 for sup in combinations(range(6),3):
  for vals in product((1,2),repeat=3):
   p=[0]*6
   for i,v in zip(sup,vals):p[i]=v
   (D if sup in supports else Dp).append(p)
 R=[list(p) for p in product((1,2),repeat=6) if p.count(1)%2==0]
 A0=[]
 for i in range(6):
  for v in (1,2):
   p=[0]*6;p[i]=v;A0.append(p)
 A1=D+R;A2=Dp+R
 assert (len(A0),len(A1),len(A2))==(12,112,112)
 def enc(p):return sum(v*3**(len(p)-1-i) for i,v in enumerate(p))
 def third_p(a,b):return enc([(-(x+y))%3 for x,y in zip(a,b)])
 def points(C):return [enc(p) for p in C]
 def valid(C):
  S={tuple(p) for p in C}
  return len(S)==len(C) and all(tuple(((-x-y)%3) for x,y in zip(a,b)) not in S for i,a in enumerate(C) for b in C[i+1:])
 for C in (A0,A1,A2):assert valid(C)
 # Extendable-collection conditions, including x=y in condition 1.
 for x in A0:
  for y in A0:
   z=third_p(x,y)
   assert z not in set(points(A1+A2))
 for x in A0:
  for y in A1:
   z=third_p(x,y)
   assert z not in set(points(A2))
 # One point from each of three layers: leading coordinates also sum to zero.
 C7=[[0]+p for p in A0]+[[1]+p for p in A1]+[[2]+p for p in A2]
 ids7=points(C7);assert len(ids7)==236 and valid(C7)
 C8=[[0]+p for p in C7];ids8=points(C8);assert len(ids8)==236 and valid(C8)
 save(str(out/'cap7-236.txt'),ids7,'Edel/Tyrrell extended product; big-endian 7D ids')
 save(str(out/'seed8-fixed236.txt'),ids8,'236-point cap fixed in coordinate layer zero')
 print('PASS cap7 size 236; embedded cap8 layer0 size 236')
if __name__=='__main__':main()
