import itertools, sys, time
src=open('full9e.py').read().split("if __name__")[0]
exec(src)
def pairs_for(R):
    Rm=0
    for w in R: Rm|=1<<enc(w)
    sg=[]
    for s in range(16807):
        if (Rm>>s)&1: continue
        b=N[s]&Rm
        if b.bit_count()==1: sg.append((s,(b&-b).bit_length()-1))
    return [(dec(r),dec(q)) for q,r in sg]
Rset={enc(w) for w in Rlist}
singles8=[(s,(N[s]&sum(1<<enc(w) for w in Rlist)).bit_length()-1) for s in range(16807) if s not in Rset and (N[s]&sum(1<<enc(w) for w in Rlist)).bit_count()==1]
# R2
old=(6,4,5,3,2); new=(6,5,5,3,1)
R2=[new if w==old else w for w in Rlist]
p2=pairs_for(R2)
t=time.time(); ts=solve_pairs(p2,X0); sol=ts.solve()
print('R2 pairs',len(p2),'SAT',sol is not None,'%.1fs'%(time.time()-t),flush=True)
if sol:
    C=build_from_sol(p2,R2,X0,sol,R2); print('R2 size',len(C),'valid',check(C),flush=True)
# 50 candidates: PAIRS8 + (r8,q8)
Rsetidx={enc(w) for w in Rlist}
blockers={}
for s in range(16807):
    if s in Rsetidx: continue
    m=N[s]; bl=[]
    while m:
        b=(m&-m).bit_length()-1; m&=m-1
        if b in Rsetidx: bl.append(b)
    blockers[s]=frozenset(bl)
D8={enc(r) for r,q in PAIRS8}; Q8={enc(q) for r,q in PAIRS8}
cands=[(r,q) for r in range(16807) if r not in D8 for q,bl in blockers.items() if q not in Q8 and bl and bl<=(D8|{r})]
print('cands',len(cands),flush=True)
nsat=0
for (r8i,q8i) in cands:
    r8=dec(r8i); q8=dec(q8i)
    pairs=PAIRS8+[(r8,q8)]
    ts=solve_pairs(pairs,X0); sol=ts.solve()
    if sol is not None:
        nsat+=1
        C=build_from_sol(pairs,Rlist,X0,sol,Rlist)
        ok=check(C)
        print('SAT cand',r8,q8,'size',len(C),'valid',ok,flush=True)
        if ok and len(C)>134753:
            print('FOUND',len(C),r8,q8,flush=True)
print('sat',nsat)
