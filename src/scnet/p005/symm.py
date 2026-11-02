import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, collections
P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
S=set(P)
def add(p,t): return tuple((a+b)%3 for a,b in zip(p,t))
def sub(p,t): return tuple((a-b)%3 for a,b in zip(p,t))
best=[]
# translation intersections
for i in range(1,3**8):
    t=[]; x=i
    for k in range(8): t.append(x%3); x//=3
    t=tuple(t)
    c=sum(1 for p in P if add(p,t) in S)
    best.append((c,t))
best.sort(reverse=True)
print('top translations:', best[:10])
# difference multiplicities
diff=collections.Counter()
for a in P:
    for b in P:
        if a!=b: diff[sub(a,b)]+=1
print('top differences:', diff.most_common(8))
print('num distinct differences:', len(diff), 'expected random ~', 6560)
# check affine spans of random subsets? compute rank of all points over F3
import itertools
def rank(pts):
    # Gaussian elimination over F3 on 8 columns
    rows=[list(p) for p in pts]
    r=0
    for c in range(8):
        piv=None
        for i in range(r,len(rows)):
            if rows[i][c]%3: piv=i;break
        if piv is None: continue
        rows[r],rows[piv]=rows[piv],rows[r]
        inv=1 if rows[r][c]%3==1 else 2
        rows[r]=[(v*inv)%3 for v in rows[r]]
        for i in range(len(rows)):
            if i!=r and rows[i][c]%3:
                f=rows[i][c]%3
                rows[i]=[(rows[i][j]-f*rows[r][j])%3 for j in range(8)]
        r+=1
    return r
base=P[0]
print('affine rank:', rank([sub(p,base) for p in P[1:]]))
