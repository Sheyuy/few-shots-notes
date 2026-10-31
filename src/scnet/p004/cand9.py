import itertools, sys
sys.setrecursionlimit(300000)
from model import Rlist,N,enc5,dec5,w1,tr,PAIRS8,solve_pairs,build,is_valid

DEL8={enc5(r) for r,_ in PAIRS8}
Rm=0
for w in Rlist: Rm|=1<<enc5(w)

# pool(D8): 不在R、且封堵全在D8 内
def pool(D):
    Dm=0
    for x in D: Dm|=1<<enc5(x)
    out=[]
    for s in range(16807):
        if (Rm>>s)&1: continue
        if (N[s]&Rm)&~Dm: continue
        out.append(dec5(s))
    return out

print('D8 的 pool =',[tuple(p) for p in pool([dec5(x) for x in DEL8])])

def gen_candidates_for_9th():
    r"""枚举第9对的所有候选: 删除 r9 ∈ R\D8，其 q9 的R-封堵 ⊆ D8∪{r9}。
       返回 (r9, q9) 的列表，q9 不在D8∪pool(D8)∪R中（否则与现有16元素重复或不在自由区）。"""
    V8 = DEL8 | {enc5(x) for x in pool([dec5(x) for x in DEL8])}
    cands=[]
    for r9 in Rlist:
        e9=enc5(r9)
        if e9 in DEL8: continue
        Dm=0
        for x in DEL8: Dm|=1<<x
        Dm|=1<<e9
        for s in range(16807):
            if (Rm>>s)&1: continue
            if (N[s]&Rm)&~Dm: continue
            q9=dec5(s)
            if enc5(q9) in V8: continue   # 必须是新的自由点
            cands.append((r9,q9))
    return cands

cands=gen_candidates_for_9th()
print(r'第9对候选数（r9∈R\D8, q9新自由点）=',len(cands))
from collections import Counter
print('按 r9 分布:',Counter(r for r,q in cands).most_common(10))
# 同时给出: 若第9对两个元素都允许来自pool(D9)（即D8∪{r9}的pool），还有多少
# 检查 pool(D8) 大小
pl=pool([dec5(x) for x in DEL8])
print('|pool(D8)| =',len(pl))
