import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, collections
P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
S=set(P)
# all nonzero linear functionals up to scalar (first nonzero coord =1)
best=[]
for f in range(1,3**8):
    d=[]; t=f
    for k in range(8): d.append(t%3); t//=3
    # normalize first nonzero to 1
    first=next((x for x in d if x!=0), None)
    if first!=1: continue
    cnt=collections.Counter(sum(d[i]*p[i] for i in range(8))%3 for p in P)
    dist=tuple(cnt.get(i,0) for i in range(3))
    if 0 in dist and max(dist)>=250:
        best.append((dist,d))
print('functionals with a zero slice and >=250 in one slice:', len(best))
for b in best[:10]: print(b)
# check balanced distributions
bal=[(tuple(collections.Counter(sum(d[i]*p[i] for i in range(8))%3 for p in P).get(i,0) for i in range(3)),d) for d in []]
