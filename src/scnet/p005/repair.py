import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, itertools, random, sys

cert = json.load(open(str(P005/'baseline'/'certificate.json')))
def idx(p):
    i=0
    for d in p: i=3*i+d
    return i
S0 = sorted(idx(p) for p in cert['points'])
Sset=set(S0)
N=3**8
D=[[ (i//(3**(7-k)))%3 for k in range(8)] for i in range(N)]
def third(a,b):
    return sum(((-(D[a][k]+D[b][k]))%3)*(3**(7-k)) for k in range(8))

# precompute pair list for each outside x with small count
conf=[0]*N
for i,a in enumerate(S0):
    for b in S0[i+1:]:
        conf[third(a,b)]+=1

outside=[x for x in range(N) if x not in Sset]
low=sorted(outside, key=lambda x: conf[x])[:40]
print('lowest conf:', [(x,conf[x]) for x in low[:20]])

def pairs_for(x,S):
    out=[]
    for i,a in enumerate(S):
        b=third(x,a)
        if b in S and a<b: out.append((a,b))
    return out

def build(S):
    c=[0]*N
    for i,a in enumerate(S):
        for b in S[i+1:]:
            c[third(a,b)]+=1
    return c

def try_repair(x, conf_x):
    pairs=pairs_for(x,Sset)
    assert len(pairs)==conf_x
    verts=sorted(set(v for p in pairs for v in p))
    # enumerate subsets of verts up to size 5 that hit all pairs
    best=0
    covers=[]
    for k in range(0,6):
        for H in itertools.combinations(verts,k):
            Hs=set(H)
            if all(a in Hs or b in Hs for a,b in pairs):
                covers.append(H)
        if covers: break  # minimal size only
    print(f'x={x} conf={conf_x} pairs={pairs} verts={len(verts)} min covers={len(covers)} size={len(covers[0]) if covers else None}')
    for H in covers:
        if len(H)>4: continue
        S=[p for p in S0 if p not in H]
        S.append(x); Sset2=set(S)
        c=build(S)
        addable=[p for p in range(N) if p not in Sset2 and c[p]==0]
        need=len(H)-1
        if len(addable)<need: continue
        # random greedy selection
        for trial in range(200):
            random.shuffle(addable)
            T=S[:]; ok=True
            for p in addable:
                if len(T)==513: break
                # check p addable wrt T
                good=True
                for a in T:
                    b=third(p,a)
                    if b in set(T) and b!=a:
                        good=False;break
                if good: T.append(p)
            if len(T)>=513:
                print('FOUND 513 via',x,H); 
                open('found.idx','w').write('\n'.join(map(str,T))+'\n')
                return True
        best=max(best,len(addable))
    return False

for x in low[:32]:
    if try_repair(x,conf[x]):
        sys.exit(0)
print('none')
