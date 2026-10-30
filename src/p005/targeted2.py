import json, random, sys, time
from pysat.solvers import Cadical195
from pysat.card import CardEnc, EncType

pts = json.load(open("/workspace/few-shots/problems/p005-cap-set-8d/baseline/certificate.json"))["points"]
def idx(p):
    x=0
    for v in p: x=x*3+v
    return x
S=sorted(idx(p) for p in pts)
inS=[False]*6561
for x in S: inS[x]=True
dig=[None]*6561
for x in range(6561):
    y=x; d=[0]*8
    for i in range(7,-1,-1): d[i]=y%3; y//=3
    dig[x]=d
def add(a,b):
    da,db=dig[a],dig[b]; r=0
    for i in range(8): r=r*3+(da[i]+db[i])%3
    return r
neg=[0]*6561
for x in range(6561):
    d=dig[x]; r=0
    for i in range(8): r=r*3+((-d[i])%3)
    neg[x]=r
# full pair counts
pc0=[0]*6561
for i in range(len(S)):
    a=S[i]
    for j in range(i+1,len(S)):
        b=S[j]
        pc0[add(a,b)]+=1
print("ready", flush=True)

def maxcap(A, need, tl=2.0):
    # exact: does A contain a cap of size >= need?
    aset=set(A); m=len(A)
    vars={p:i+1 for i,p in enumerate(A)}
    clauses=[]
    # enumerate triples a<b<c in A with sum 0
    for i in range(m):
        a=A[i]
        for j in range(i+1,m):
            b=A[j]
            c=neg[add(a,b)]
            if c in aset and c>b:
                clauses.append([-vars[a],-vars[b],-vars[c]])
    enc=CardEnc.atleast(lits=list(range(1,m+1)), bound=need, top_id=m, encoding=EncType.seqcounter)
    clauses.extend(enc.clauses)
    s=Cadical195(bootstrap_with=clauses)
    if s.solve():
        model=set(l for l in s.get_model() if l>0)
        sel=[A[i] for i in range(m) if (i+1) in model]
        return sel
    return None

random.seed(int(time.time()))
t0=time.time()
budget=float(sys.argv[1]) if len(sys.argv)>1 else 240
best=512
trials=0
ks=[1,2,3,4,5,6,7]
while time.time()-t0<budget:
    k=random.choice(ks)
    trials+=1
    R=random.sample(S,k)
    Rset=set(R)
    pc=pc0[:]  # copy
    # remove points of R from the pair counts
    for r in R:
        for a in S:
            if a==r or a in Rset: continue
            pc[add(r,a)]-=1
    A=[]
    for p in range(6561):
        if inS[p] and p not in Rset: continue
        if pc[neg[p]]==0: A.append(p)
    if len(A)<k+1 or len(A)>60: continue
    sel=maxcap(A,k+1)
    if sel is None: continue
    allp=[x for x in S if x not in Rset]+sel
    st=set(allp)
    if len(allp)!=len(st): continue
    ok=True
    for i in range(len(allp)):
        for j in range(i+1,len(allp)):
            if neg[add(allp[i],allp[j])] in st: ok=False; break
        if not ok: break
    if ok:
        best=len(allp)
        print("FOUND", best, "k",k,"|A|",len(A),"trial",trials, flush=True)
        import os; os.makedirs("cand513",exist_ok=True)
        json.dump({"points":[dig[x][:] for x in allp]}, open("cand513/certificate.json","w"))
        if best>=513: sys.exit(0)
    if trials%10==0:
        print("trials",trials,"best",best,"k",k,"|A|",len(A),"t=%.1f"%(time.time()-t0), flush=True)
print("done",trials,"best",best)