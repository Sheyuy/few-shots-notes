#!/usr/bin/env python3
"""Exact minimum vertex cover of conflict lines, optionally forbidding fixed-point deletion."""
import argparse
from pathlib import Path
import numpy as np
from scipy.optimize import milp,Bounds,LinearConstraint
from scipy.sparse import coo_array
from caplib import load,save,third,valid_cap

def main():
 ap=argparse.ArgumentParser();ap.add_argument('candidate');ap.add_argument('--fixed');ap.add_argument('--prefix',required=True);ap.add_argument('--time',type=float,default=60);args=ap.parse_args()
 C=list(dict.fromkeys(load(args.candidate)));S=set(C);fixed=set(load(args.fixed)) if args.fixed else set();pos={x:i for i,x in enumerate(C)};lines=[]
 for i,a in enumerate(C):
  for b in C[i+1:]:
   z=third(a,b)
   if z in S and z>a and z>b:lines.append((a,b,z))
 removable=[x for x in C if x not in fixed];ri={x:i for i,x in enumerate(removable)};rows=[];cols=[]
 for r,tri in enumerate(lines):
  vs=[x for x in tri if x not in fixed]
  if not vs:raise SystemExit(f'conflict line entirely fixed: {tri}')
  for x in vs:rows.append(r);cols.append(ri[x])
 A=coo_array((np.ones(len(rows)),(rows,cols)),shape=(len(lines),len(removable))).tocsc()
 res=milp(c=np.ones(len(removable)),integrality=np.ones(len(removable)),bounds=Bounds(0,1),constraints=LinearConstraint(A,1,np.inf),options={'time_limit':args.time})
 if res.x is None:raise SystemExit(f'no cover incumbent: status {res.status} {res.message}')
 cover=[removable[i] for i,v in enumerate(res.x) if v>.5];repaired=sorted(S-set(cover))
 save(args.prefix+'-cover.txt',cover,f'status {res.status}')
 save(args.prefix+'-repaired.txt',repaired,f'size {len(repaired)}')
 print('RESULT status',res.status,'conflict_lines',len(lines),'cover',len(cover),'dual',res.mip_dual_bound,'repaired',len(repaired),'valid',valid_cap(repaired),'fixed_retained',len(fixed&set(repaired)))
if __name__=='__main__':main()
