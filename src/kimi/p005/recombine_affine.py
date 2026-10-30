#!/usr/bin/env python3
"""Generate deterministic unions of random affine images of the 512-point cap."""
import argparse,random
from pathlib import Path
from caplib import load,save,decode,encode,valid_cap

def mat_vec(M,p):return [sum(M[i][j]*p[j] for j in range(8))%3 for i in range(8)]
def rand_GL(rng):
 while True:
  M=[[rng.randrange(3) for _ in range(8)] for _ in range(8)];A=[r[:] for r in M];rank=0
  for c in range(8):
   q=next((q for q in range(rank,8) if A[q][c]),None)
   if q is None:continue
   A[rank],A[q]=A[q],A[rank];inv=1 if A[rank][c]==1 else 2;A[rank]=[(v*inv)%3 for v in A[rank]]
   for j in range(8):
    if j!=rank and A[j][c]:
     f=A[j][c];A[j]=[(A[j][i]-f*A[rank][i])%3 for i in range(8)]
   rank+=1
  if rank==8:return M
def affine_cap(C,seed):
 rng=random.Random(seed);M=rand_GL(rng);t=[rng.randrange(3) for _ in range(8)]
 out=sorted(encode([(v+t[i])%3 for i,v in enumerate(mat_vec(M,decode(x)))]) for x in C)
 assert valid_cap(out);return out
def main():
 ap=argparse.ArgumentParser();ap.add_argument('baseline');ap.add_argument('outdir');args=ap.parse_args();out=Path(args.outdir);out.mkdir(parents=True,exist_ok=True);base=load(args.baseline)
 for k,seed in [(2,41001),(2,41002),(3,41003)]:
  copies=[base]+[affine_cap(base,seed+j) for j in range(k-1)];pool=sorted(set().union(*map(set,copies)))
  save(str(out/f'recombine-k{k}-{seed}-pool.txt'),pool,f'k {k} seed {seed}')
  print('PASS',k,seed,'pool',len(pool),'intersections',[len(set(base)&set(c)) for c in copies[1:]])
if __name__=='__main__':main()
