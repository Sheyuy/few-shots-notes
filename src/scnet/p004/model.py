"""Audited p004 paired-strip model (Python stdlib).

C = B x B plus H(j,i)=(X[i],pair[j][choice]) and
V(j,i)=(pair[j][choice],X[i]).  The caller supplies independent X and
independent baseR. All pair endpoints must avoid the closed neighbourhood of B.

All HH, VV, HV conflicts, INCLUDING duplicate selected points, are forbidden.
Independent X makes HH/VV cross-index constraints vacuous. HV includes i==i'
and same pair/same endpoint: it never skips these cases.

For strict 2-SAT, X points with identical endpoint-neighbourhood signatures can
be merged exactly. Inactive signature-zero points take all r choices. Optional
slot deletion requires either the uncompressed model or multiplicity weights:
copying the most populated feasible H/V pattern across equal-signature points
preserves every constraint and cannot decrease the number of selected slots.
Unweighted deletion budgets on compressed variables would be incorrect.

This module creates no files and does not run a search on import.
"""
from functools import lru_cache
from pathlib import Path
import itertools
import random

HALF=7**5
def _r367_path():
    import os
    env=os.environ.get('P004_R367')
    cands=[Path(env)] if env else []
    cands+=[Path(__file__).resolve().with_name('R367.txt'),
            Path('/tmp/lbs-repo2/c7/R367.txt')]
    for c in cands:
        if c.exists():return c
    raise SystemExit('R367.txt not found; set P004_R367 to its path')
R_PATH=_r367_path()
Rlist=[tuple(map(int,line.split())) for line in R_PATH.read_text().splitlines() if line.strip()]
PAIRS8=[((1,3,4,4,6),(2,3,5,4,6)),((3,4,0,3,5),(2,4,6,3,5)),((5,3,1,3,4),(5,3,2,3,5)),((4,4,6,1,6),(5,4,6,0,6)),((6,0,6,4,5),(6,1,6,5,5)),((0,3,5,6,5),(6,3,5,0,5)),((6,4,3,4,0),(6,4,2,4,6)),((6,4,5,3,2),(6,5,5,3,1))]

def enc5(d):
    if isinstance(d,int):
        if not 0<=d<HALF: raise ValueError('point out of range')
        return d
    if len(d)!=5 or any(type(x) is not int or not 0<=x<7 for x in d):raise ValueError('bad five-vector')
    out=0
    for v in d:out=out*7+v
    return out

def dec5(i):
    i=enc5(i);out=[0]*5
    for k in range(4,-1,-1):i,out[k]=divmod(i,7)
    return tuple(out)

DIGITS=[dec5(i) for i in range(HALF)]

def _masks():
    prev=[sum(1<<x for x in ((d-1)%7,d,(d+1)%7)) for d in range(7)]
    width=7
    for _ in range(2,6):
        prev=[(tail<<(((d-1)%7)*width))|(tail<<(d*width))|(tail<<(((d+1)%7)*width)) for d in range(7) for tail in prev]
        width*=7
    return prev

N=_masks()

def near(a,b):return bool((N[enc5(a)]>>enc5(b))&1)
def w1(a,b):return near(a,b)
def tr(w):return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])
def bits(mask):
    while mask:
        bit=mask&-mask;yield bit.bit_length()-1;mask^=bit

def independent5(points):
    points=[enc5(p) for p in points]
    if len(points)!=len(set(points)):return False
    mask=sum(1<<p for p in points)
    return all(N[p]&mask==1<<p for p in points)

class TwoSAT:
    """Node 2*v+a denotes the assignment v==a, not a negated clause literal."""
    def __init__(self,n):self.n=n;self.g=[[] for _ in range(2*n)];self.rg=[[] for _ in range(2*n)];self.forbidden=[]
    def forbid(self,v1,a1,v2,a2):
        assert 0<=v1<self.n and 0<=v2<self.n
        a1=int(a1);a2=int(a2);assert a1 in (0,1) and a2 in (0,1)
        self.forbidden.append((v1,a1,v2,a2))
        u=2*v1+a1;v=2*v2+a2
        self.g[u].append(v^1);self.rg[v^1].append(u)
        self.g[v].append(u^1);self.rg[u^1].append(v)
    def satisfies(self,sol):
        return len(sol)==self.n and all(not(sol[u]==a and sol[v]==b) for u,a,v,b in self.forbidden)
    def solve(self):
        n2=2*self.n;seen=bytearray(n2);order=[]
        for root in range(n2):
            if seen[root]:continue
            seen[root]=1;stack=[(root,0)]
            while stack:
                u,pos=stack[-1]
                if pos==len(self.g[u]):order.append(u);stack.pop();continue
                w=self.g[u][pos];stack[-1]=(u,pos+1)
                if not seen[w]:seen[w]=1;stack.append((w,0))
        comp=[-1]*n2;number=0
        for root in reversed(order):
            if comp[root]>=0:continue
            comp[root]=number;stack=[root]
            while stack:
                u=stack.pop()
                for w in self.rg[u]:
                    if comp[w]<0:comp[w]=number;stack.append(w)
            number+=1
        self.contradictory_variables=[i for i in range(self.n) if comp[2*i]==comp[2*i+1]]
        if self.contradictory_variables:return None
        sol=[comp[2*i]<comp[2*i+1] for i in range(self.n)]
        assert self.satisfies(sol),'SCC assignment violates its own clauses'
        return sol

class StripModel:
    def __init__(self,pairs,X,baseR=None,compress=True,validate=True):
        self.pairs=[tuple(enc5(p) for p in pair) for pair in pairs]
        if any(len(pair)!=2 for pair in self.pairs):raise ValueError('not a pair')
        self.X=[enc5(x) for x in X];self.d=len(self.pairs);self.nx=len(self.X);self.compress=compress
        self.elements=[x for pair in self.pairs for x in pair]
        if len(set(self.elements))!=2*self.d:raise ValueError('pair endpoints must be distinct')
        if not independent5(self.X):raise ValueError('this kernel requires independent X')
        if baseR is None:baseR=Rlist
        self.baseR=[enc5(x) for x in baseR]
        self.deleted={p[0] for p in self.pairs}
        self.B=[x for x in self.baseR if x not in self.deleted]
        if validate:
            if not independent5(self.baseR):raise ValueError('baseR is not independent')
            if not self.deleted<=set(self.baseR):raise ValueError('deleted point absent from baseR')
            Bmask=sum(1<<x for x in self.B)
            if any(N[e]&Bmask for e in self.elements):raise ValueError('endpoint conflicts with B')
        signatures=[sum(1<<e for e,p in enumerate(self.elements) if (N[p]>>x)&1) for x in self.X]
        if compress:
            self.patterns=sorted(set(signatures)-{0});pos={s:i for i,s in enumerate(self.patterns)}
            self.position=[pos[s] if s else -1 for s in signatures]
        else:
            self.patterns=[s for s in signatures if s];self.position=[];at=0
            for s in signatures:
                self.position.append(at if s else -1)
                if s:at+=1
        self.ng=len(self.patterns);self.nactive=sum(s!=0 for s in signatures)
        self.ninactive=self.nx-self.nactive
        self.nvars=2*self.d*self.ng
        self.ts=TwoSAT(self.nvars)
        self._generate()
    def H(self,j,i):return j*self.ng+i
    def V(self,j,i):return self.d*self.ng+j*self.ng+i
    def _generate(self):
        # Independent X: only same-index HH and VV remain.
        for j in range(self.d):
            for k in range(j+1,self.d):
                for a,b in itertools.product((0,1),repeat=2):
                    if not near(self.pairs[j][a],self.pairs[k][b]):continue
                    for i in range(self.ng):
                        self.ts.forbid(self.H(j,i),a,self.H(k,i),b)
                        self.ts.forbid(self.V(j,i),a,self.V(k,i),b)
        # H(j,x)=(x,a), V(k,y)=(b,y): x is near b AND y is near a.
        contacts=[[i for i,s in enumerate(self.patterns) if (s>>e)&1] for e in range(2*self.d)]
        for a in range(2*self.d):
            for b in range(2*self.d):
                for i in contacts[b]:
                    for iy in contacts[a]:
                        self.ts.forbid(self.H(a//2,i),a%2,self.V(b//2,iy),b%2)
    def expand(self,sol):
        if len(sol)!=self.nvars:raise ValueError('assignment length mismatch')
        full=[0]*(2*self.d*self.nx)
        for j in range(self.d):
            for i,pos in enumerate(self.position):
                if pos>=0:
                    full[j*self.nx+i]=sol[self.H(j,pos)]
                    full[self.d*self.nx+j*self.nx+i]=sol[self.V(j,pos)]
        return full
    def solve(self):
        sol=self.ts.solve()
        return None if sol is None else self.expand(sol)
    @property
    def raw_size(self):return len(self.B)**2+2*self.d*self.nx
    def points(self,full):
        if len(full)!=2*self.d*self.nx:raise ValueError('full assignment length mismatch')
        C={a*HALF+b for a in self.B for b in self.B};count=len(C)
        for j,pair in enumerate(self.pairs):
            for i,x in enumerate(self.X):
                h=full[j*self.nx+i];v=full[self.d*self.nx+j*self.nx+i]
                if h is not None:C.add(x*HALF+pair[int(h)]);count+=1
                if v is not None:C.add(pair[int(v)]*HALF+x);count+=1
        if len(C)!=count:raise ValueError('selected slots contain duplicate points')
        return C

class AsymmetricStripModel:
    """Paired strips with separate independent auxiliary sets XH and XV.

    H(j,i)=(XH[i], endpoint), V(j,k)=(endpoint, XV[k]).  Contact signatures
    and their multiplicities are compressed separately on the two sides.
    Identical-signature H blocks (or V blocks) may copy their most populated
    feasible pattern; this preserves all constraints and does not reduce score.
    Thus weighted optional-slot optimization is exact for this fixed family.
    """
    def __init__(self,pairs,XH,XV,baseR=None,compress=True):
        self.pairs=[tuple(enc5(x) for x in pair) for pair in pairs]
        if any(len(pair)!=2 for pair in self.pairs):raise ValueError('not a pair')
        self.d=len(self.pairs);self.elements=[x for pair in self.pairs for x in pair]
        if len(set(self.elements))!=2*self.d:raise ValueError('pair endpoints must be distinct')
        self.XH=[enc5(x) for x in XH];self.XV=[enc5(x) for x in XV]
        if not independent5(self.XH) or not independent5(self.XV):raise ValueError('both auxiliary sets must be independent')
        self.baseR=[enc5(x) for x in (Rlist if baseR is None else baseR)]
        if not independent5(self.baseR):raise ValueError('baseR must be independent')
        self.deleted={p[0] for p in self.pairs}
        if not self.deleted<=set(self.baseR):raise ValueError('deleted point absent from baseR')
        self.B=[x for x in self.baseR if x not in self.deleted]
        Bmask=sum(1<<x for x in self.B)
        if any(N[e]&Bmask for e in self.elements):raise ValueError('endpoint conflicts with B')
        def side(X):
            signatures=[sum(1<<j for j,e in enumerate(self.elements) if (N[e]>>x)&1) for x in X]
            if compress:
                patterns=sorted(set(signatures)-{0});pos={s:i for i,s in enumerate(patterns)}
                positions=[pos[s] if s else -1 for s in signatures]
            else:
                patterns=[s for s in signatures if s];positions=[];i=0
                for s in signatures:
                    positions.append(i if s else -1)
                    if s:i+=1
            weights=[0]*len(patterns)
            for i in positions:
                if i>=0:weights[i]+=1
            return patterns,positions,weights
        self.ph,self.ih,self.wh=side(self.XH);self.pv,self.iv,self.wv=side(self.XV)
        self.nh=len(self.ph);self.nv=len(self.pv);self.nvars=self.d*(self.nh+self.nv)
        self.weights=self.wh*self.d+self.wv*self.d
        self.active_slots=sum(self.weights)
        self.total_slots=self.d*(len(self.XH)+len(self.XV))
        self.inactive_slots=self.total_slots-self.active_slots
        self.ts=TwoSAT(self.nvars)
        for j in range(self.d):
            for k in range(j+1,self.d):
                for a,b in itertools.product((0,1),repeat=2):
                    if near(self.pairs[j][a],self.pairs[k][b]):
                        for i in range(self.nh):self.ts.forbid(self.H(j,i),a,self.H(k,i),b)
                        for i in range(self.nv):self.ts.forbid(self.V(j,i),a,self.V(k,i),b)
        ch=[[i for i,s in enumerate(self.ph) if (s>>e)&1] for e in range(2*self.d)]
        cv=[[i for i,s in enumerate(self.pv) if (s>>e)&1] for e in range(2*self.d)]
        for a in range(2*self.d):
            for b in range(2*self.d):
                for i in ch[b]:
                    for k in cv[a]:self.ts.forbid(self.H(a//2,i),a%2,self.V(b//2,k),b%2)
    def H(self,j,i):return j*self.nh+i
    def V(self,j,i):return self.d*self.nh+j*self.nv+i
    def expand(self,sol):
        if len(sol)!=self.nvars:raise ValueError('assignment length mismatch')
        nh=len(self.XH);nv=len(self.XV);full=[0]*self.total_slots
        for j in range(self.d):
            for i,pos in enumerate(self.ih):
                if pos>=0:full[j*nh+i]=sol[self.H(j,pos)]
            for i,pos in enumerate(self.iv):
                if pos>=0:full[self.d*nh+j*nv+i]=sol[self.V(j,pos)]
        return full
    def solve(self):
        sol=self.ts.solve()
        return None if sol is None else self.expand(sol)
    @property
    def raw_size(self):return len(self.B)**2+self.total_slots
    def points(self,full):
        if len(full)!=self.total_slots:raise ValueError('full assignment length mismatch')
        C={a*HALF+b for a in self.B for b in self.B};count=len(C);nh=len(self.XH);nv=len(self.XV)
        for j,pair in enumerate(self.pairs):
            for i,x in enumerate(self.XH):
                a=full[j*nh+i]
                if a is not None:C.add(x*HALF+pair[int(a)]);count+=1
            for i,x in enumerate(self.XV):
                a=full[self.d*nh+j*nv+i]
                if a is not None:C.add(pair[int(a)]*HALF+x);count+=1
        if len(C)!=count:raise ValueError('selected slots contain duplicate points')
        return C

def solve_pairs(pairs,X):
    """Compatibility wrapper; returns full assignments, as older callers expect."""
    return StripModel(pairs,X)

def build(pairs,X,sol,baseR=None):
    model=StripModel(pairs,X,baseR=baseR)
    C=model.points(sol)
    return {DIGITS[p//HALF]+DIGITS[p%HALF] for p in C},[DIGITS[b] for b in model.B]

def is_valid(C):
    groups={}
    for t in C:
        if isinstance(t,int):p,s=divmod(t,HALF)
        else:p=enc5(t[:5]);s=enc5(t[5:])
        groups.setdefault(p,[]).append(s)
    members={};reach={}
    for p,ss in groups.items():
        m=u=0
        for s in ss:
            if (u>>s)&1:return False
            m|=1<<s;u|=N[s]
        members[p]=m;reach[p]=u
    for p,u in reach.items():
        for q in bits(N[p]):
            if q>p and members.get(q,0)&u:return False
    return True

def natural_candidates():
    R=[enc5(x) for x in Rlist];Rm=sum(1<<x for x in R)
    D={enc5(p[0]) for p in PAIRS8};Dm=sum(1<<x for x in D)
    old={enc5(x) for p in PAIRS8 for x in p};order={r:i for i,r in enumerate(R)}
    out=[]
    for q in range(HALF):
        if (Rm>>q)&1 or q in old:continue
        extra=(N[q]&Rm)&~Dm
        if extra.bit_count()==1:
            r=extra.bit_length()-1
            if r not in D:out.append((DIGITS[r],DIGITS[q]))
    out.sort(key=lambda p:(order[enc5(p[0])],enc5(p[1])))
    return out

def selftest():
    rng=random.Random(310317)
    for a,b in itertools.product(range(7),repeat=2):
        expected=(a-b)%7 in (0,1,6)
        assert near(a,b)==expected and near(a,b)==near(b,a)
    for _ in range(10000):
        a=rng.randrange(HALF);b=rng.randrange(HALF)
        assert near(a,b)==all((x-y)%7 in (0,1,6) for x,y in zip(DIGITS[a],DIGITS[b]))
    for _ in range(300):
        n=rng.randrange(1,8);t=TwoSAT(n)
        for _ in range(rng.randrange(1,30)):t.forbid(rng.randrange(n),rng.randrange(2),rng.randrange(n),rng.randrange(2))
        brute=next((x for x in itertools.product((0,1),repeat=n) if t.satisfies(x)),None)
        answer=t.solve();assert (answer is None)==(brute is None)
    cases=0
    # Low-dimensional concentrated points give dense conflicts and equal-point cases.
    while cases<160:
        points=rng.sample(range(49),4)
        if near(points[0],points[2]):continue
        pairs=[(points[0],points[1]),(points[2],points[3])]
        X=[rng.randrange(49)]
        if rng.randrange(2):
            choices=[x for x in range(49) if not near(x,X[0])]
            X.append(rng.choice(choices))
        compact=StripModel(pairs,X,baseR=[points[0],points[2]],compress=True)
        expanded=StripModel(pairs,X,baseR=[points[0],points[2]],compress=False)
        def brute_ok(sol):
            ps=[];n=len(X)
            for j,pair in enumerate(pairs):
                for i,x in enumerate(X):ps.extend((x*HALF+pair[sol[j*n+i]],pair[sol[2*n+j*n+i]]*HALF+x))
            return all(not(near(p//HALF,q//HALF) and near(p%HALF,q%HALF)) for p,q in itertools.combinations(ps,2))
        brute=any(brute_ok(s) for s in itertools.product((0,1),repeat=4*len(X)))
        c=compact.solve();e=expanded.solve();assert (c is not None)==brute==(e is not None)
        if c is not None:assert brute_ok(c)
        cases+=1
    assert independent5(Rlist)
    X=[tr(w) for w in Rlist];X[267]=(1,5,6,3,5);assert independent5(X)
    print('SELFTEST adjacency=10049 random_2SAT=300 direct_extension_cases=%d PASS'%cases,flush=True)

if __name__=='__main__':
    selftest()
    X=[tr(w) for w in Rlist];X[267]=(1,5,6,3,5)
    m=StripModel(PAIRS8,X);sol=m.solve();assert sol is not None
    C=m.points(sol);assert is_valid(C) and len(C)==134753
    print('CONTROL pairs=8 X=%d active_X=%d contact_signatures=%d variables=%d clauses=%d reconstructed_points=%d'%(len(X),m.nactive,m.ng,m.nvars,len(m.ts.forbidden),len(C)),flush=True)
