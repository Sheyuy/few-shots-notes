import itertools
from pathlib import Path
import numpy as np


def rref_mod3(A):
    A=np.array(A,dtype=np.int8)%3
    rows,cols=A.shape;pivots=[];r=0
    for c in range(cols):
        candidates=np.flatnonzero(A[r:,c])
        if not len(candidates):continue
        q=r+int(candidates[0]);A[[r,q]]=A[[q,r]]
        if A[r,c]==2:A[r]=(2*A[r])%3
        for q in range(rows):
            if q!=r and A[q,c]:A[q]=(A[q]-A[q,c]*A[r])%3
        pivots.append(c);r+=1
        if r==rows:break
    return A,pivots


def kernel_basis(functionals):
    V=np.array(functionals,dtype=np.int8)%3;D=V.shape[1]
    R,pivots=rref_mod3(V)
    free=[c for c in range(D) if c not in pivots]
    B=np.zeros((D,len(free)),dtype=np.int8)
    for j,f in enumerate(free):
        B[f,j]=1
        for i,p in enumerate(pivots):B[p,j]=(-R[i,f])%3
    assert np.all((V@B.astype(np.int16))%3==0)
    assert np.array_equal(B[free],np.eye(len(free),dtype=np.int8))
    return B,free


def functionals9():
    F=[]
    for i in range(9):
        for suffix in itertools.product(range(3),repeat=8-i):
            F.append((0,)*i+(1,)+suffix)
    return np.array(F,dtype=np.int8)


def best_affine_section(M,functionals,F9=None):
    M=np.array(M,dtype=np.int8);V=np.array(functionals,dtype=np.int8)
    B,free=kernel_basis(V)
    vals=(V.astype(np.int16)@M.astype(np.int16))%3
    ids=np.flatnonzero(np.all(vals==0,axis=0))
    Y=M[free][:,ids]
    assert np.all((B.astype(np.int16)@Y.astype(np.int16))%3==M[:,ids])
    m=len(ids)
    if F9 is None:F9=functionals9()
    dots=(F9.astype(np.int16)@Y.astype(np.int16))%3
    counts=(dots==0).sum(axis=1)
    k=int(np.argmin(counts));best=m-int(counts[k])
    return {'m':m,'min_k':int(counts[k]),'best':best,'ids':ids,'Y':Y,'counts':counts,'k':k,'F9':F9,'B':B}


def extract_affine(result):
    m=result['m'];Y=result['Y'];k=result['k'];g=result['F9'][k].astype(np.int16)
    v=(g@Y.astype(np.int16))%3
    pivot=int(np.flatnonzero(g)[0]);points=[]
    for j in np.flatnonzero(v):
        col=(Y[:,j].astype(np.int16)*(2 if v[j]==1 else 1))%3
        assert (g@col)%3==2
        points.append(int(np.delete(col,pivot)@(3**np.arange(8,dtype=np.int64))))
    return points,pivot
