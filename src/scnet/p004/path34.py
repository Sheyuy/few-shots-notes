import sys, itertools, collections
NS=16807
one=collections.defaultdict(list)
pair=collections.defaultdict(list)
three=collections.defaultdict(list)
adj=collections.defaultdict(set)
t0=0
for line in open('/tmp/p004_blockers.txt'):
    v=list(map(int,line.split()))
    if not v: continue
    m=v[0]; x=v[1]; bl=tuple(sorted(v[2:2+m]))
    if m==1: one[bl[0]].append(x)
    elif m==2:
        pair[bl].append(x); adj[bl[0]].add(bl[1]); adj[bl[1]].add(bl[0])
    else: three[bl].append(x)
print("one",sum(len(v) for v in one.values()),"pair",sum(len(v) for v in pair.values()),"three",sum(len(v) for v in three.values()),"edges",sum(len(v) for v in adj.values())//2, flush=True)
# enumerate paths a-b-c
paths=set()
for b in adj:
    nb=list(adj[b])
    for i in range(len(nb)):
        for j in range(i+1,len(nb)):
            a=nb[i]; c=nb[j]
            D=tuple(sorted((a,b,c)))
            paths.add(D)
print("distinct D",len(paths), flush=True)
def digs(x):
    d=[0]*10
    for i in range(9,-1,-1): d[i]=x%7; x//=7
    return d
def adj10(x,y):
    dx=digs(x); dy=digs(y)
    for i in range(10):
        e=(dx[i]-dy[i])%7
        if e>1 and e<6: return False
    return True
def has4(points):
    n=len(points)
    if n<4: return None
    am=[0]*n
    for i in range(n):
        for j in range(i+1,n):
            if adj10(points[i],points[j]):
                am[i]|=1<<j; am[j]|=1<<i
    # backtracking for independent set of size 4
    def rec(start, chosen_mask, avail):
        if len(chosen_mask)==4: return list(chosen_mask)
        # prune
        cnt=0
        m=avail
        while m:
            cnt+=1; m&=m-1
        if cnt < 4-len(chosen_mask): return None
        m=avail
        while m:
            b=(m&-m).bit_length()-1; m&=m-1
            if b<start: continue
            r=rec(b+1, chosen_mask+[b], avail & ~am[b] & ~((1<<(b+1))-1))
            if r: return r
        return None
    return rec(0,[],(1<<n)-1)
found=0; checked=0; maxn=0
for idx,D in enumerate(paths):
    a,b,c=D
    pool=set()
    for arr in (one[a],one[b],one[c]): pool.update(arr)
    pool.update(pair.get((a,b),[]))
    pool.update(pair.get((a,c),[]))
    pool.update(pair.get((b,c),[]))
    pool.update(three.get(D,[]))
    n=len(pool)
    if n>maxn: maxn=n
    if n>=4:
        checked+=1
        r=has4(list(pool))
        if r:
            found+=1
            print("FOUND D",D,"pool",n,"chosen",r, flush=True)
            if found>20: break
    if idx%50000==0: print("...",idx,"checked",checked,"maxn",maxn, flush=True)
print("done paths",len(paths),"with>=4",checked,"found",found,"maxn",maxn, flush=True)
