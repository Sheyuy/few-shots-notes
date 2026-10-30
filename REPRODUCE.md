# 完全可复现指南

本指南包含所有实验的**精确命令、源码路径、依赖项、预期输出**。所有源码在 `src/` 目录下。

## 环境要求

```bash
# Python 3.12+ (标准库足够用于 few-shots verifier)
python3 --version  # 3.12.5 测试通过

# GCC (C 语言搜索程序)
gcc --version  # 13.2 测试通过，需支持 -march=native

# Python SAT 求解器 (可选，用于 SAT 实验)
pip3 install python-sat
# 包含 CaDiCaL、Kissat、Minisat22、Glucose4 等

# few-shots 仓库 (用于基线复现与 verifier)
git clone https://github.com/few-shots/few-shots.git
cd few-shots
python3 tools/run_tests.py  # 可选，15 秒
```

## 1. p003 · R(4,6) 下界

### 1.1 基线复现

```bash
cd few-shots
python3 problems/p003-ramsey-r46/verify/verify.py problems/p003-ramsey-r46/baseline
# 预期输出: {"valid":true,"score":35,"canonical_hash":"..."}
```

### 1.2 37 个已知 35 点图扩展判定

**源码**: `src/p003/ext_sat.py`

```bash
# 下载 37 个 35 点图
cd /workspace/few-shots-work/p003
curl -sL --max-time 60 -o r46_35some.g6 https://users.cecs.anu.edu.au/~bdm/data/r46_35some.g6

# 解析为文本格式
python3 - <<'EOF'
def g6_to_adj(s):
    s=s.strip()
    n=ord(s[0])-63
    bits=[]
    for ch in s[1:]:
        v=ord(ch)-63
        for k in range(5,-1,-1):
            bits.append((v>>k)&1)
    adj=[[0]*n for _ in range(n)]
    idx=0
    for j in range(1,n):
        for i in range(j):
            if bits[idx]: adj[i][j]=adj[j][i]=1
            idx+=1
    return adj
import os
lines=[l for l in open('r46_35some.g6') if l.strip()]
os.makedirs('seeds',exist_ok=True)
for gi,l in enumerate(lines):
    a=g6_to_adj(l)
    n=len(a)
    with open(f'seeds/g{gi}.txt','w') as f:
        f.write(f"{n}\n")
        for r in a: f.write(''.join(map(str,r))+"\n")
EOF

# 运行 SAT (每个种子 ~0.1 秒)
python3 src/p003/ext_sat.py
# 预期输出: 37 个 "False", 最后一行 "all 37 unsat"
```

### 1.3 全量 SAT 编码

**源码**: `src/p003/sat.py`

```bash
cd /workspace/few-shots-work/p003
PYTHONPATH=/workspace/few-shots-work/pylibs timeout 1800 python3 src/p003/sat.py > sat.log 2>&1
# 预期: 2006697 clauses, 求解时间不确定 (可能需要数小时)
# 输出: solve True 或 False，若 True 生成 sat_certificate.json
```

### 1.4 度数分层 SAT (推荐先跑)

**源码**: `src/p003/sat_deg2.py`

```bash
cd /workspace/few-shots-work/p003
# 对每个合法度数 d=11..17 运行 (每个 15-30 分钟)
for d in 11 12 13 14 15 16 17; do
  timeout 1800 python3 src/p003/sat_deg2.py $d kissat > deg_$d.log 2>&1 &
done
# 预期: 输出 "d $N solve False" 或 "d $N solve True"
# 若 True，生成 deg${d}_certificate.json，立即用 verifier 复核
```

### 1.5 自同构商图 SAT (最快排除路径)

**源码**: `src/p003/symq.py`

```bash
cd /workspace/few-shots-work/p003
# 支持的循环型: inv0 (18 对换), inv4 (4 不动点+16 对换), ord3 (12 3-循环),
#              ord4 (9 4-循环), ord6 (6 6-循环), ord2fix2, ord3fix 等
for kind in inv0 ord3 ord4 ord6; do
  timeout 1200 python3 src/p003/symq.py $kind 1 kissat > symq_$kind.log 2>&1 &
done
# 预期: inv0 输出 "solve False" (~2 分钟)，其余可能超时
```

### 1.6 局部搜索 (C 语言)

**源码**: `src/p003/search.c`

```bash
cd /workspace/few-shots-work/p003
gcc -O3 -march=native -o search src/p003/search.c -lm
# 模式 B: 固定 35 点图，搜索第 36 点的邻域
./search B seeds/g0.txt 30 > out_B.txt 2> log_B.txt
# 预期: "modeB best=..." 若找到 0 违反，生成 found_b.txt

# 复核 (必须用官方 verifier)
python3 /workspace/few-shots/problems/p003-ramsey-r46/verify/verify.py cand036
```

### 1.7 结果验证脚本

**源码**: `src/p003/verify_found.py`

```bash
# 验证 SAT 生成的证书
python3 src/p003/verify_found.py deg13_certificate.json
# 输出 n=36，调用官方 verifier 复核
```

## 2. p004 · C7 香农容量

### 2.1 基线复现

```bash
cd few-shots
python3 problems/p004-shannon-capacity-c7/verify/verify.py problems/p004-shannon-capacity-c7/baseline
# 预期输出: {"valid":true,"score":134753,...}
```

### 2.2 极大性判定

**源码**: `src/p004/maximal.c`

```bash
cd /workspace/few-shots-work/p004
# 导出基线点集
cd few-shots
python3 - <<'EOF'
import json
pts=json.load(open("problems/p004-shannon-capacity-c7/baseline/certificate.json"))["points"]
def to7(s):
    x=0
    for ch in s: x=x*7+int(ch)
    return x
with open("/workspace/few-shots-work/p004/points.txt","w") as f:
    for s in pts: f.write(str(to7(s))+"\n")
EOF

# 编译运行
gcc -O3 -march=native -o maximal src/p004/maximal.c
./maximal points.txt
# 预期输出: "MAXIMAL" (0.6 秒)
```

### 2.3 1 换 2 交换搜索

**源码**: `src/p004/exchange.c`

```bash
cd /workspace/few-shots-work/p004
gcc -O3 -march=native -o exchange src/p004/exchange.c
./exchange points.txt
# 预期输出: "NONE" (18 秒)
```

### 2.4 2 换 3 交换搜索

**源码**: `src/p004/exchange2.c`

```bash
cd /workspace/few-shots-work/p004
gcc -O3 -march=native -o exchange2 src/p004/exchange2.c
./exchange2 points.txt
# 预期输出: "NONE" (11 秒)
```

### 2.5 3 换 4 交换搜索

**源码**: `src/p004/exchange3.c`

```bash
cd /workspace/few-shots-work/p004
gcc -O3 -march=native -o exchange3 src/p004/exchange3.c
./exchange3 points.txt
# 预期输出: "NONE" (15 秒)
```

## 3. p005 · 8 维帽集

### 3.1 基线复现

```bash
cd few-shots
python3 problems/p005-cap-set-8d/verify/verify.py problems/p005-cap-set-8d/baseline
# 预期输出: {"valid":true,"score":512,...}
```

### 3.2 定向精确搜索

**源码**: `src/p005/targeted2.py`

```bash
cd /workspace/few-shots-work/p005
# 导出基线点集索引
cd few-shots
python3 - <<'EOF'
import json
pts=json.load(open("problems/p005-cap-set-8d/baseline/certificate.json"))["points"]
def idx(p):
    x=0
    for v in p: x=x*3+v
    return x
with open("/workspace/few-shots-work/p005/init.txt","w") as f:
    f.write(str(len(pts))+"\n")
    for p in pts: f.write(str(idx(p))+"\n")
EOF

# 运行 (PYTHONPATH 指向 pysat 安装位置)
PYTHONPATH=/workspace/few-shots-work/pylibs python3 src/p005/targeted2.py 300
# 预期输出: "done trials ... best=512"
```

## 4. p006 · LABS-100

### 4.1 基线复现

```bash
cd few-shots
python3 problems/p006-labs-100/verify/verify.py problems/p006-labs-100/baseline
# 预期输出: {"valid":true,"score":578,...}
```

### 4.2 模拟退火

**源码**: `src/p006/sa.c`

```bash
cd /workspace/few-shots-work/p006
gcc -O3 -march=native -o sa src/p006/sa.c -lm
./sa 300 300
# 预期输出: "done bestE=..." (最好 806)
```

## 5. p001 · 11 维接吻数

### 5.1 基线复现

```bash
cd few-shots
python3 problems/p001-kissing-number-11d/verify/verify.py problems/p001-kissing-number-11d/baseline
# 预期输出: {"valid":true,"score":604,...}
```

### 5.2 POCS 投影可行性检验

**源码**: `src/p001/pocs.c`

```bash
cd /workspace/few-shots-work/p001
# 导出向量
cd few-shots
python3 - <<'EOF'
import json
V=json.load(open("problems/p001-kissing-number-11d/baseline/certificate.json"))["vectors"]
with open("/workspace/few-shots-work/p001/vecs.txt","w") as f:
    for v in V: f.write(' '.join(f'{p} {q}' for p,q in v)+'\n')
EOF

# 编译运行
gcc -O3 -march=native -o pocs src/p001/pocs.c -lm
./pocs vecs.txt
# 预期输出: "no feasible found, best violation ..." (~3.81)
```

## 6. p008 · AES S-box 电路

### 6.1 基线复现

```bash
cd few-shots
python3 problems/p008-aes-sbox-circuit/verify/verify.py problems/p008-aes-sbox-circuit/shots/huangxin-design/aes102-field-plane-offsets
# 预期输出: {"valid":true,"score":102,...}
```

### 6.2 真值表去重检查

```bash
cd few-shots
python3 - <<'EOF'
import json
p="problems/p008-aes-sbox-circuit/shots/huangxin-design/aes102-field-plane-offsets/certificate.json"
c=json.load(open(p))
gates=c["gates"]; outs=c["outputs"]
tabs={}
for i in range(8):
    v=0
    for b in range(256):
        if (b>>i)&1: v|=1<<b
    tabs[f"x{i}"]=v
def bit(op,a,b):
    if op=="AND": return a&b
    if op=="XOR": return a^b
    if op=="XNOR": return (~(a^b))&((1<<256)-1)
for g in gates:
    tabs[g["out"]]=bit(g["op"],tabs[g["a"]],tabs[g["b"]])
seen={}
dups=[]
for g in gates:
    t=tabs[g["out"]]
    if t in seen: dups.append((seen[t],g["out"]))
    else: seen[t]=g["out"]
print("duplicate-truth-table gate wires:",len(dups))
print("dead gates:", [g["out"] for g in gates if g["out"] not in set(outs+[g["a"] for g in gates]+[g["b"] for g in gates)])
# 预期输出: 0 个重复，0 个死门
EOF
```

## 关键依赖

| 依赖 | 版本 | 用途 |
| --- | --- | --- |
| Python | 3.12+ | few-shots verifier, SAT 封装 |
| pip python-sat | 1.9.dev17 | SAT 求解器 (CaDiCaL/Kissat/Minisat22) |
| GCC | 12+ | C 语言搜索程序，需支持 `-march=native` |
| few-shots | main | 基线复现、verifier 复核 |

## 常见问题

**Q: SAT 求解器一直不返回结果怎么办？**
A: p003 的全量 SAT 有 200 万条子句，可能需要数小时。推荐先跑"度数分层"或"自同构商图"版本，变量更少、更容易判定。

**Q: 局部搜索找到了 0 违反的图，但 verifier 说无效？**
A: 这是增量计数 bug 的典型表现。我遇到过一次：`count4/indep5` 把相邻点对误当独立点对。**每次找到候选都必须用官方 verifier 复核**，不要相信搜索程序的输出。

**Q: 远端 DevEnv 怎么接入？**
A: 当前沙箱的 SSH 网关禁止端口转发（`administratively prohibited`），沙箱 MCP 也在维护中。如果恢复，可以用 `huaweicloud-devkit_huaweicloud_sandbox_connect` 获取 session_id，然后用 `sandbox_exec` 执行命令。
