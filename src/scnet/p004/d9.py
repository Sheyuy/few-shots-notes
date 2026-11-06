import sys, time, itertools, json
sys.path.insert(0,'/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004')
import model as m
import paper_construct as pc

t0=time.time()
C,B,A,D = pc.build(J0=(0,5,6))
print('d8 size',len(C),'valid',m.is_valid(C),'A',len(A),'D',len(D),round(time.time()-t0,1))
