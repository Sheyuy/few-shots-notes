import itertools, sys, json
sys.path.insert(0,'/Users/shiny/.sclaw/workspace/few-shots-work/p004')
sys.setrecursionlimit(300000)
from model import Rlist,N,enc5,dec5,w1,tr,PAIRS8,solve_pairs,build,is_valid

# 50个候选直接内联（从cand9模块逻辑重新生成，避免打印）
Rm=0
for w in Rlist: Rm|=1<<enc5(w)
DEL8={enc5(r) for r,_ in PAIRS8}
pool8=[]
for s in range(16807):
    if (Rm>>s)&1: continue
    if (N[s]&Rm)&~sum(1<<x for x in DEL8): continue
    pool8.append(dec5(s))
V8=DEL8|{enc5(x) for x in pool8}
cands=[]
for r9 in Rlist:
    e9=enc5(r9)
    if e9 in DEL8: continue
    Dm=sum(1<<x for x in DEL8)|(1<<e9)
    for s in range(16807):
        if (Rm>>s)&1: continue
        if (N[s]&Rm)&~Dm: continue
        if s in V8: continue
        cands.append((r9,dec5(s)))
print('候选数',len(cands),flush=True)

X0=[tr(w) for w in Rlist]; X0[267]=(1,5,6,3,5)
Xset={enc5(x) for x in X0}

sat=0; done=0; t0=__import__('time').time()
for idx,(r9,q9) in enumerate(cands):
    # 前提: X0 与 18 个对元素不交
    if enc5(q9) in Xset:
        print(f'{idx}: X与q9相交,跳过',flush=True); continue
    pairs=PAIRS8+[(r9,q9)]
    ts=solve_pairs(pairs,X0)
    sol=ts.solve(); done+=1
    if sol is not None:
        C,B=build(pairs,X0,sol)
        v=is_valid(C)
        sat+=1
        print(f'{idx}: SAT 规模={len(C)} valid={v} r9={r9} q9={q9}',flush=True)
    if idx%5==0: print(f'...{idx} 已用时{__import__("time").time()-t0:.0f}s',flush=True)
print(f'完成 done={done} sat={sat}')
