import sys, contextlib, io, collections
sys.path.insert(0,'/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004')
import model as m, paper_construct as pc
from model import N, HALF, bits
with contextlib.redirect_stdout(io.StringIO()):
    import cand9
cands=cand9.gen_candidates_for_9th()

def first_conflict(C):
    groups={}
    for t in C:
        p,s=divmod(t,HALF); groups.setdefault(p,[]).append(s)
    members={};reach={}
    for p,ss in groups.items():
        mm=u=0
        for s in ss:
            if (u>>s)&1: return ('row',p,s,None)
            mm|=1<<s; u|=N[s]
        members[p]=mm;reach[p]=u
    for p,u in reach.items():
        for q in bits(N[p]):
            if q>p and members.get(q,0)&u:
                s=(members[q]&u).bit_length()-1
                return ('cross',p,q,s)
    return None

for i in range(6):
    r9,q9=cands[i]
    pairs=list(m.PAIRS8)+[(tuple(r9),tuple(q9))]
    C,B,A,D=pc.build(pairs=pairs,J0=(0,5,6))
    fc=first_conflict(C)
    kind=fc[0] if fc else 'valid'
    print(i, tuple(r9), tuple(q9), kind, fc, flush=True)
