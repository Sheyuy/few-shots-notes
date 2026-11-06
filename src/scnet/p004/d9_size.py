import sys, contextlib, io, collections
sys.path.insert(0,'/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004')
import model as m, paper_construct as pc
with contextlib.redirect_stdout(io.StringIO()):
    import cand9
cands=cand9.gen_candidates_for_9th()
for i,(r9,q9) in enumerate(cands[:6]):
    pairs=list(m.PAIRS8)+[(tuple(r9),tuple(q9))]
    for J0 in [(0,5,6),(0,5,8),(0,6,8),(5,6,8)]:
        C,B,A,D=pc.build(pairs=pairs,J0=J0)
        print(i, tuple(r9), tuple(q9), 'J0',J0,'size',len(C),'A',len(A),'D',len(D),'valid', (len(C)==134770 and m.is_valid(C)))
