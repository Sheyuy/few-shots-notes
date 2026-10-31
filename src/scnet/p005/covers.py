import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, collections
P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
def idx(p):
    i=0
    for d in p: i=3*i+d
    return i
S0=sorted(idx(p) for p in P); Sset=set(S0); N=6561
D=[[ (i//(3**(7-k)))%3 for k in range(8)] for i in range(N)]
def third(a,b): return sum(((-(D[a][k]+D[b][k]))%3)*(3**(7-k)) for k in range(8))

def pairs_for(x):
    out=[]
    for a in S0:
        b=third(x,a)
        if b in Sset and a<b: out.append((a,b))
    return out

# min vertex cover by brute force branching for small pair sets
def min_cover(pairs, limit=8):
    pairs=[tuple(sorted(p)) for p in pairs]
    if not pairs: return 0
    # branch on first pair
    a,b=pairs[0]
    rest=[p for p in pairs if a not in p and b not in p]
    r1=1+min_cover(rest,limit-1)
    rest2=[p for p in pairs if b not in p]  # cover a
    r2=1+min_cover([p for p in pairs if a not in p],limit-1) # cover b
    # above is messy; do proper recursion below
    return min(r1,r2)

def mvc(pairs):
    pairs=list(pairs)
    if not pairs: return 0
    a,b=pairs[0]
    # choose a
    ra=[p for p in pairs if a not in p]
    # choose b
    rb=[p for p in pairs if b not in p]
    return 1+min(mvc(ra),mvc(rb))

hist=collections.Counter()
small=[]
for x in range(N):
    if x in Sset: continue
    ps=pairs_for(x)
    k=mvc(ps)
    hist[k]+=1
    if k<=5: small.append((x,k,len(ps)))
print('min cover histogram:', sorted(hist.items()))
print('points with cover<=4:', [(x,k,n) for x,k,n in small if k<=4][:40])
print('points with cover<=5:', [(x,k,n) for x,k,n in small if k<=5][:60])
