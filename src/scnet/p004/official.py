import itertools, json, sys
sys.setrecursionlimit(200000)

Rlist=[tuple(map(int,l.split())) for l in open('/tmp/lbs-repo2/c7/R367.txt') if l.strip()]

def enc5(d):
    r=0
    for v in d: r=r*7+v
    return r
def dec5(i):
    d=[0]*5
    for k in range(4,-1,-1): d[k]=i%7; i//=7
    return tuple(d)
N=[0]*16807
for s in range(16807):
    d=dec5(s); m=0
    for off in itertools.product((-1,0,1),repeat=5):
        m|=1<<enc5([(d[i]+off[i])%7 for i in range(5)])
    N[s]=m
def w1(a,b): return all(min((x-y)%7,(y-x)%7)<=1 for x,y in zip(a,b))
def padj5(i,j): return i==j or ((N[i]>>j)&1)
def tr(w): return ((2-w[1])%7,w[3],w[0],(2-w[2])%7,w[4])

PAIRS8=[((1,3,4,4,6),(2,3,5,4,6)),((3,4,0,3,5),(2,4,6,3,5)),((5,3,1,3,4),(5,3,2,3,5)),((4,4,6,1,6),(5,4,6,0,6)),((6,0,6,4,5),(6,1,6,5,5)),((0,3,5,6,5),(6,3,5,0,5)),((6,4,3,4,0),(6,4,2,4,6)),((6,4,5,3,2),(6,5,5,3,1))]

# ---- 完整独立集检验（用于校验构造出的 C）----
def is_independent(C):
    T={}
    for t in C:
        p=tuple(t[:5]); s=tuple(t[5:])
        T.setdefault(p,set()).add(s)
    for p,ss in T.items():
        for s in ss:
            for s2 in ss:
                if s!=s2 and w1(s,s2): return False
        # 组内：s 的邻域不能覆盖组内其它点
    # 跨组
    U={}
    for p,ss in T.items():
        u=0
        for s in ss: u|=N[enc5(s)]
        U[p]=u
    for p,ss in T.items():
        m=N[enc5(p)]
        while m:
            b=(m&-m).bit_length()-1; m&=m-1
            if b==enc5(p): continue
            if b in U:
                for s in ss:
                    if (U_set:=T.get(dec5(b))) and any(w1(s,s2) for s2 in U_set): return False
    return True

def official():
    """逐字复现官方脚本，输出 (C, assignment) 用于校验模型"""
    R={w:0 for w in Rlist}
    PAIRS=PAIRS8
    deleted={r for r,_ in PAIRS}
    B=tuple(w for w in R if w not in deleted)
    X=[tr(w) for w in Rlist]
    X[267]=(1,5,6,3,5)
    J0=[0,5,6]; J1=[1,2,3,4]
    PH=[PAIRS[j][0] for j in J0]+[PAIRS[j][1] for j in J1]
    PV=[PAIRS[j][1] for j in J0]+[PAIRS[j][0] for j in J1]
    def conflicts(x,P):
        for y in P:
            if all(min((xi-yi)%7,(yi-xi)%7)<=1 for xi,yi in zip(x,y)): return True
        return False
    A={x for x in X if conflicts(x,PV)}
    D={x for x in X if conflicts(x,PH)}
    C=[x[0]+x[1] for x in itertools.product(B,repeat=2)]
    # 记录官方赋值: H[j][i] = 0(r) 或 1(q)
    H={}; V={}
    for i,x in enumerate(X):
        for j in range(8):
            ih=((j in J0) and (x in A)) or ((j in J1) and (x in D))
            iv=((j in J0) and (x in D)) or ((j in J1) and (x in A))
            C.append(x+PAIRS[j][ih])
            C.append(PAIRS[j][iv]+x)
            H[(j,i)]=ih; V[(j,i)]=iv
    return C,H,V,X,B

if __name__=='__main__':
    C,H,V,X,B=official()
    print('官方构造规模 =',len(C))
    # 检查是否为独立集（用和 verifier 一致的规则）
    bad=0
    T={}
    for t in C:
        p=tuple(t[:5]); s=tuple(t[5:])
        T.setdefault(p,[]).append(s)
    for p,ss in T.items():
        for a,b in itertools.combinations(ss,2):
            if w1(a,b): bad+=1
        m=N[enc5(p)]
        while m:
            q=(m&-m).bit_length()-1; m&=m-1
            if q in T:
                for s1 in ss:
                    for s2 in T[q]:
                        if w1(s1,s2): bad+=1
    print('官方构造内部相邻对数（应为0）:',bad)
    # 存成证书格式以备官方 verifier
    json.dump({'points':[''.join(map(str,t)) for t in C]},open('/tmp/official_cert.json','w'))
    print('已写出 /tmp/official_cert.json')
