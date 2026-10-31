import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json, itertools
P=[tuple(p) for p in json.load(open(str(P005/'baseline'/'certificate.json')))['points']]
def idx(p):
    i=0
    for d in p: i=3*i+d
    return i
S0=sorted(idx(p) for p in P); Sset=set(S0)
N=6561
D=[[ (i//(3**(7-k)))%3 for k in range(8)] for i in range(N)]
def third(a,b): return sum(((-(D[a][k]+D[b][k]))%3)*(3**(7-k)) for k in range(8))
conf=[0]*N
for i,a in enumerate(S0):
    for b in S0[i+1:]: conf[third(a,b)]+=1
def build(S):
    c=[0]*N
    for i,a in enumerate(S):
        for b in S[i+1:]: c[third(a,b)]+=1
    return c
def pairs_for(x):
    out=[]
    for a in S0:
        b=third(x,a)
        if b in Sset and a<b: out.append((a,b))
    return out
x=250
pairs=pairs_for(x); print('pairs',pairs)
for ci,H in enumerate(itertools.product(*pairs)):
    Hs=set(H)
    T=[p for p in S0 if p not in Hs]+[x]
    c=build(T); Ts=set(T)
    A=[p for p in range(N) if p not in Ts and c[p]==0]
    print(ci, 'H=',H, 'addable=',len(A), A[:12])
