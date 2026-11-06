import itertools, random, collections
Rlist=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]
def enc(d):
    r=0
    for v in d: r=r*7+v
    return r
def dec(i):
    d=[0]*5
    for k in range(4,-1,-1): d[k]=i%7; i//=7
    return tuple(d)
N=[0]*16807
for s in range(16807):
    d=dec(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        m|=1<<enc([(d[i]+off[i])%7 for i in range(5)])
    N[s]=m
def stats(Rmask):
    singles=[]; hist=collections.Counter()
    for s in range(16807):
        if (Rmask>>s)&1: continue
        c=(N[s]&Rmask).bit_count()
        hist[c]+=1
        if c==1: singles.append(s)
    return singles,hist
Rmask=0
for w in Rlist: Rmask|=1<<enc(w)
singles,hist=stats(Rmask)
print('initial singles',len(singles),'hist',sorted(hist.items())[:5])
def addable(Rmask,r):
    # points q with blockers(q) subset {r}
    out=[]
    for s in range(16807):
        if (Rmask>>s)&1: continue
        b=N[s]&Rmask
        if b and (b & ~(1<<r))==0: out.append(s)
    return out
best=len(singles); cur=Rmask; cur_singles=singles
random.seed(0)
for it in range(200):
    r=random.choice(list(cur_singles))
    cand=addable(cur,r)
    if not cand: continue
    q=random.choice(cand)
    new=(cur & ~(1<<r)) | (1<<q)
    ns,h=stats(new)
    if len(ns)>=best:
        if len(ns)>best: print('iter',it,'singles',len(ns),flush=True)
        best=len(ns); cur=new; cur_singles=ns
    if best>=12: break
print('best',best)
s,h=stats(cur); print('final singles',len(s),'hist',sorted(h.items())[:5])
print('R points',[dec(x) for x in range(16807) if (cur>>x)&1][:5])
