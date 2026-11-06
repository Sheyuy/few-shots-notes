import itertools, sys, json, random
sys.path.insert(0,'/Users/shiny/.sclaw/workspace/few-shots-work/p004')
sys.setrecursionlimit(300000)
from model import Rlist,N,enc5,dec5,w1,tr,PAIRS8,solve_pairs,build,is_valid
from cand9 import gen_candidates_for_9th

# 生成9对候选: 8对原始 + 第9对
cands = gen_candidates_for_9th()
print('第9对候选数:', len(cands), flush=True)

# X = tr(R367) + patch
X0=[tr(w) for w in Rlist]; X0[267]=(1,5,6,3,5)

results = []
for idx,(r9,q9) in enumerate(cands):
    pairs = PAIRS8 + [(r9,q9)]
    try:
        ts = solve_pairs(pairs, X0)
        sol = ts.solve()
        if sol is not None:
            C,B = build(pairs, X0, sol)
            valid = is_valid(C)
            results.append((idx, r9, q9, len(C), valid))
            print(f'候选{idx}: SAT, 规模={len(C)} valid={valid}', flush=True)
    except AssertionError as e:
        print(f'候选{idx}: 前提违反 {e}', flush=True)
print('SAT的候选数:', len(results))
for r in results[:20]:
    print(r)
