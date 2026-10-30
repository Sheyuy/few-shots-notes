#!/usr/bin/env python3
"""Check archived cut arity, unique variables, and incumbent feasibility."""
import argparse
from pathlib import Path
from caplib import load

def main():
 ap=argparse.ArgumentParser();ap.add_argument('pool');ap.add_argument('seed');ap.add_argument('cuts');args=ap.parse_args()
 pool=sorted(set(load(args.pool)));seed=set(load(args.seed));count=0;max_ratio=0
 for line in Path(args.cuts).read_text().splitlines():
  if not line or line.startswith('#'):continue
  z=list(map(int,line.split()))
  if len(z)==z[0]+1:bound,n,vs=4,z[0],z[1:]
  elif len(z)==z[1]+2:bound,n,vs=z[0],z[1],z[2:]
  else:raise ValueError(f'malformed line: {line[:80]}')
  assert len(vs)==n and len(set(vs))==n and all(0<=v<len(pool) for v in vs)
  occ=sum(pool[v] in seed for v in vs)
  if occ>bound:raise AssertionError(f'seed violates cut {count}: occupancy {occ} > {bound}')
  max_ratio=max(max_ratio,occ/bound);count+=1
 print(f'PASS cuts {count}; seed satisfies every bound; max occupancy/bound {max_ratio:.6f}')
if __name__=='__main__':main()
