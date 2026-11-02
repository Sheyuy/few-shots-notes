import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, itertools, collections
P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
def idx(p):
    i=0
    for d in p: i=3*i+d
    return i
S0=sorted(idx(p) for p in P); Sset=set(S0); N=6561
D=[[ (i//(3**(7-k)))%3 for k in range(8)] for i in range(N)]
def third(a,b): return sum(((-(D[a][k]+D[b][k]))%3)*(3**(7-k)) for k in range(8))
conf=[0]*N
for i,a in enumerate(S0):
    for b in S0[i+1:]: conf[third(a,b)]+=1
def pairs_for(x):
    out=[]
    for a in S0:
        b=third(x,a)
        if b in Sset and a<b: out.append((a,b))
    return out
low=[x for x in range(N) if x not in Sset and conf[x]==4]
print('conf4 points:',len(low))
freq=collections.Counter()
trans={}
for x in low:
    ps=pairs_for(x)
    ts=[frozenset(t) for t in itertools.product(*ps)]
    trans[x]=ts
    for t in ts: freq[t]+=1
print('number of distinct transversals:',len(freq))
print('max frequency:',max(freq.values()))
top=[(c,t) for t,c in freq.items() if c>=2]
print('transversals unlocking >=2 conf4 points:',len(top))
for c,t in sorted(top,reverse=True)[:10]:
    print(c, sorted(t))
# check disjointness of pairs
alldis=True
for x in low:
    ps=pairs_for(x); v=[q for p in ps for q in p]
    if len(v)!=len(set(v)): alldis=False
print('all conf4 pairs disjoint:',alldis)
