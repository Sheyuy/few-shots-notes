import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, collections

cert = json.load(open(str(P005/'baseline'/'certificate.json')))
pts = [tuple(p) for p in cert['points']]
print('n =', len(pts), 'unique =', len(set(pts)))

def idx(p):
    i=0
    for d in p: i = 3*i + d
    return i
def digits(i):
    p=[0]*8
    for k in range(7,-1,-1):
        p[k]=i%3; i//=3
    return p

S = sorted(idx(p) for p in pts)
Sset=set(S)
N=3**8
D=[digits(i) for i in range(N)]

# conflict counts via all unordered pairs
cnt=[0]*N
pair_of=[None]*N  # for small cnt store pairs
for ii,a in enumerate(S):
    for b in S[ii+1:]:
        kd=[(-(D[a][t]+D[b][t]))%3 for t in range(8)]
        k=(kd[0]*2187+kd[1]*729+kd[2]*243+kd[3]*81+kd[4]*27+kd[5]*9+kd[6]*3+kd[7])
        cnt[k]+=1

h=collections.Counter(cnt[x] for x in range(N) if x not in Sset)
print('hist outside:', sorted(h.items())[:8], 'max', max(h))
print('min cnt outside:', min(cnt[x] for x in range(N) if x not in Sset))

def pairs_for(x):
    res=[]
    tx=[(-v)%3 for v in D[x]]
    for ii,a in enumerate(S):
        need=[(tx[t]-D[a][t])%3 for t in range(8)]
        needidx=need[0]*2187+need[1]*729+need[2]*243+need[3]*81+need[4]*27+need[5]*9+need[6]*3+need[7]
        if needidx in Sset and needidx>a:
            res.append((a,needidx))
    return res

low=[x for x in range(N) if x not in Sset and cnt[x]<=2]
print('outside cnt<=2:', len(low))
by_a=collections.Counter()
for x in low:
    ps=pairs_for(x)
    assert len(ps)==cnt[x]
    if len(ps)==1:
        for a in ps[0]: by_a[a]+=1
print('max unlocked by one removal:', by_a.most_common(5))
