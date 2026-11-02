import random, json, time
d=6; N=3**d
def digs(i):
    p=[0]*d
    for k in range(d-1,-1,-1): p[k]=i%3; i//=3
    return p
D=[digs(i) for i in range(N)]
def third(a,b):
    r=0
    for k in range(d): r=r*3+((-(D[a][k]+D[b][k]))%3)
    return r

def greedy():
    conf=[0]*N; S=[]; ins=[False]*N
    order=list(range(N)); random.shuffle(order)
    for p in order:
        if conf[p]==0:
            ins[p]=True; S.append(p)
            for q in S[:-1]: conf[third(p,q)]+=1
    return S,conf,ins

def lns(S,conf,ins,iters=200000):
    best=len(S); bestS=S[:]
    t0=time.time()
    for it in range(iters):
        r=random.randint(4,14)
        rem=random.sample(S,r)
        remset=set(rem)
        for p in rem:
            S.remove(p); ins[p]=False
            for q in S: conf[third(p,q)]-=1
        added=[]
        for _ in range(3):
            order=list(range(N)); random.shuffle(order)
            for p in order:
                if not ins[p] and conf[p]==0:
                    ins[p]=True; S.append(p); added.append(p)
                    for q in S[:-1]: conf[third(p,q)]+=1
        if len(S)>best:
            best=len(S); bestS=S[:]; print('it',it,'best',best,flush=True)
            if best>=112: return bestS
        elif len(S)<best:
            # undo
            for p in reversed(added):
                S.remove(p); ins[p]=False
                for q in S: conf[third(p,q)]-=1
            for p in rem:
                ins[p]=True; S.append(p)
                for q in S[:-1]: conf[third(p,q)]+=1
    return bestS

best=0; bestS=None
for trial in range(30):
    S,conf,ins=greedy()
    if len(S)>best:
        best=len(S); bestS=S[:]; print('greedy',trial,best,flush=True)
    if best>=112: break
if best<112:
    S,conf,ins=greedy()
    for p in bestS: pass
    # build from bestS
    conf=[0]*N; ins=[False]*N
    for p in bestS:
        ins[p]=True
        for q in bestS:
            if q!=p and q<p: conf[third(p,q)]+=1
    bestS=lns(bestS[:],conf,ins)
print('final',len(bestS))
json.dump(sorted(bestS),open('cap6_best.json','w'))
