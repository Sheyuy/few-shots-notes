import itertools, sys
Rlist=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]
def enc(d):
    r=0
    for v in d: r=r*7+v
    return r
def dec(i):
    d=[0]*5
    for k in range(4,-1,-1): d[k]=i%7; i//=7
    return tuple(d)
def conf(x,P): return any(all(min((u-v)%7,(v-u)%7)<=1 for u,v in zip(x,y)) for y in P)
N=[0]*16807
for s in range(16807):
    d=dec(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        m|=1<<enc([(d[i]+off[i])%7 for i in range(5)])
    N[s]=m
def singles(R):
    Rm=0
    for w in R: Rm|=1<<enc(w)
    out=[]
    for x in range(16807):
        if (Rm>>x)&1: continue
        b=N[x]&Rm
        if b.bit_count()==1: out.append((x,(b&-b).bit_length()-1))
    return out
def tr(w): return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])
def check(P):
    T={}
    for x in P:
        p=enc(x[:5]); s=enc(x[5:]); T.setdefault(p,set()).add(s)
    U={}
    for p,ss in T.items():
        u=0
        for s in ss: u|=N[s]
        U[p]=u
    for p,ss in T.items():
        tb=0
        for s in ss: tb|=1<<s
        for s in ss:
            if (tb & N[s]) != (1<<s): return False
        m=N[p]
        while m:
            b=(m&-m).bit_length()-1; m&=m-1
            if b==p: continue
            if b in T and (tb & U[b]): return False
    return True
old=(6,4,5,3,2); new=(6,5,5,3,1)
R1=[new if w==old else w for w in Rlist]
sg=singles(R1)
pairs=[(dec(r),dec(q)) for q,r in sg]
def build(J0,patch=None):
    d=len(pairs); deleted={enc(r) for r,q in pairs}
    B=[w for w in R1 if enc(w) not in deleted]
    X=[tr(w) for w in R1]
    if patch:
        for i,x in enumerate(X):
            if x==patch[0]: X[i]=patch[1]; break
    Xs=set(X)
    J1=[j for j in range(d) if j not in J0]
    PH=[pairs[j][0] for j in J0]+[pairs[j][1] for j in J1]
    PV=[pairs[j][1] for j in J0]+[pairs[j][0] for j in J1]
    A=set(x for x in Xs if conf(x,PV)); Ds=set(x for x in Xs if conf(x,PH))
    C=set()
    for b1 in B:
        for b2 in B: C.add(b1+b2)
    for x in X:
        for j in range(d):
            inJ0=j in J0
            s=((inJ0 and x in A) or ((not inJ0) and x in Ds))
            s2=((inJ0 and x in Ds) or ((not inJ0) and x in A))
            C.add(x+pairs[j][s]); C.add(pairs[j][s2]+x)
    return C
if __name__=='__main__':
    patch=None
    found=0
    for k in (3,4,5):
        for J0 in itertools.combinations(range(9),k):
            C=build(set(J0),patch)
            if check(C): print('VALID patch J0',J0,len(C),flush=True); found+=1
    print('found',found)
