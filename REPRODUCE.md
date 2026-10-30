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
A: 已有一条成功路径（CodeArts 轮次，DevEnv_337466，640 核 / 2 TB / Ubuntu 22.04 aarch64）：
- SSH 直连授权仅 5 分钟；用 `ssh -M -S /tmp/p005.sock -N -f -o ControlPersist=12h ...` 建立 ControlMaster 复用；
- 远端没有 `python3`/`pip`/`gcc`：可 `curl get-pip.py` 或直接解压 pip wheel 到用户目录（不要写系统目录）；
- `ortools` 最新版无 aarch64 wheel，必须 `ortools==9.11.4210`；
- 可本地 `pip download ortools==9.11.4210 --platform manylinux2014_aarch64 --python-version 310 --implementation cp --abi cp310 --dest wheels/`，再 `tar czf - -C wheels . | ssh ... 'tar xzf - -C /workspace/wheels'`，解压到 `PYTHONPATH` 目录；
- 远端任务用 `setsid + nohup` + 看门狗自恢复。详见 `docs/REPORT_CODEARTS.md` 第 4 节。

## 9. CodeArts 轮次（p001 / p003 / p005 + 远端 640 核）

完整报告：`docs/REPORT_CODEARTS.md`；源码：`src/codearts/`。

依赖：

```bash
pip install numpy
pip install ortools==9.11.4210   # x86_64 可用最新版；aarch64 必须该版本
pip install python-sat           # 仅 p003
```

### 9.1 p005 · 分层精确最大团

源码：`src/codearts/p005/layers.py`、`layers_exact.py`

```bash
python3 layers_exact.py
# 预期：layer 0 |C| 200 best clique 192；layer 1/2 |C| 229 best clique 160
```

### 9.2 p005 · 跨层交换 / 联合重启 / 仿射重分层

```bash
python3 exchange.py      # 960 次单点交换，最好总点数 511
python3 layer_search.py  # 300 次两层联合重启，最好 512
python3 affine.py        # 120 个随机仿射分层 + 逐层精确，最好 512
```

### 9.3 p005 · 全局 CP-SAT（重点）

源码：`src/codearts/p005/global_cpsat_hint.py`；需要 `baseline.json`（`{"points": [[0..2]*8, ...]}`）。

```bash
# 修改脚本里的路径后运行
python3 global_cpsat_hint.py
# 预期日志：lines 7173360 -> hint added 512 -> model ready ~183s -> Solve
# 64 worker 稳定约 137 GB；256 worker 必 OOM
```

### 9.4 p001 · 覆盖半径量化

源码：`src/codearts/p001/coverage.py`

```bash
python3 coverage.py
# 预期：150 起点的最小 max cos ≈ 0.5776（54.7°），无一次 ≤ 0.5
```

### 9.5 p001 · norm²=72 排查

源码：`src/codearts/p001/norm_check.py`、`pure72.py`

```bash
python3 norm_check.py   # 修正 DP 采样后：一般候选 10 万个，86 个内积可行但范数 b≠0
python3 pure72.py       # 纯形式 Σpq=0 采样 20 万个，合法 0 个
```

注意：`norm_check.py` 的 DP 表必须用 `s - w` 递推（与采样一致）；用 `s + w` 会生成几乎全是单坐标的“假随机”向量。

### 9.6 p003 · 37 个 35 点图加顶点 SAT

源码：`src/codearts/p003/sat_all.py`、`r46_35some.g6`

```bash
python3 sat_all.py
# 预期：37 个图全部 unsat，每图 <1 秒；总计 1167 条 3/5 元子句、35 个变量
```


## 10. Kimi p005 独立轮次（射影截面 / 非对称搜索 / 精确壳层）

报告：`docs/REPORT_KIMI_P005.md`。以下命令均从本仓库根目录执行。点文件统一采用与官方证书一致的大端三进制。

```bash
# 精确模型需要 NumPy / SciPy；本次复核环境为 SciPy 1.16.2。
python3 -m pip install numpy scipy
export PYTHONPATH="$PWD/src/kimi/p005${PYTHONPATH:+:$PYTHONPATH}"
R=${R:-/tmp/p005-kimi-replay}
mkdir -p "$R"
sha256sum -c data/kimi/p005/MANIFEST.sha256
```

### 10.1 无需求解器的合法性检查

```bash
python3 src/kimi/p005/check_points.py \
  data/kimi/p005/baseline512.txt --expect-size 512 --expect-bad 0
python3 src/kimi/p005/check_points.py \
  data/kimi/p005/seeds496/seed{0,1,2,3,4,5}.txt \
  --expect-size 496 --expect-bad 0
for k in 29 34 22 10 7 4; do
  python3 src/kimi/p005/check_points.py \
    "data/kimi/p005/nearmiss/near${k}.txt" --expect-size 513 --expect-bad "$k"
done
```

### 10.2 六个 496 点种子的精确 11 删除邻域

```bash
for i in 0 1 2 3 4 5; do
  python3 src/kimi/p005/make_shell.py \
    "data/kimi/p005/seeds496/seed${i}.txt" 11 "$R/seed${i}-d11.txt"
  python3 src/kimi/p005/shell_milp.py \
    "$R/seed${i}-d11.txt" "data/kimi/p005/seeds496/seed${i}.txt" \
    --remove 11 --minsize 496 --time 180 --prefix "$R/seed${i}-d11"
done
# 每次预期：BUILT 704 lines 2240；随后 status 0 / Optimal，
# objective 496，dual_bound 496，valid True。
```

这证明的是各种子自身的有界邻域，不是全局上界。仓库已有实际输出：`logs/kimi/p005/exact496-seed*-d11.log`。

### 10.3 原 512 点基线的壳层

```bash
for d in 4 8 10 11 12; do
  python3 src/kimi/p005/make_shell.py \
    data/kimi/p005/baseline512.txt "$d" "$R/baseline-d${d}.txt"
  python3 src/kimi/p005/shell_milp.py \
    "$R/baseline-d${d}.txt" data/kimi/p005/baseline512.txt \
    --remove "$d" --minsize 512 --time 180 --prefix "$R/baseline-d${d}"
done
# 全部预期 status 0 / Optimal，objective=dual_bound=512。

# 第 13 壳层可以重跑，但原 180 秒记录不是排除证明。
python3 src/kimi/p005/make_shell.py \
  data/kimi/p005/baseline512.txt 13 "$R/baseline-d13.txt"
python3 src/kimi/p005/shell_milp.py \
  "$R/baseline-d13.txt" data/kimi/p005/baseline512.txt \
  --remove 13 --minsize 512 --time 180 --prefix "$R/baseline-d13"
# 原日志：status 1，incumbent 512，dual_bound 534，未判定。
```

### 10.4 近 miss 的精确点覆盖

```bash
for k in 29 34 22 10 7 4; do
  python3 src/kimi/p005/analyze_nearmiss.py \
    "data/kimi/p005/nearmiss/near${k}.txt" --time 120 \
    --output "$R/near${k}-repaired.txt"
done
# 最小覆盖依次为 29、17、7、3、2、1；合法修复大小依次为
# 484、496、506、510、511、512。

python3 src/kimi/p005/analyze_nearmiss.py \
  data/kimi/p005/nearmiss/near22.txt \
  --baseline data/kimi/p005/baseline512.txt --keep-new \
  --output "$R/near22-keep-new-repaired.txt"
# 强制保留 7 个非基线点时，精确覆盖为 22，合法大小为 491。
```

只有 `status 0 / Optimal` 才能读作精确最小覆盖；时间限制状态不能这么解释。

### 10.5 全空间“一删二换”穷举

```bash
g++ -O3 -std=c++20 src/kimi/p005/exchange_1to2.cpp -o "$R/exchange_1to2"
seq 0 6560 > "$R/full6561.txt"
OUTPUT_PREFIX="$R/exchange" "$R/exchange_1to2" "$R/full6561.txt" \
  data/kimi/p005/seeds496/seed{0,1,2,3,4,5}.txt
# 每个种子：outside 6065，pairs 18389080，found 0。
```

### 10.6 射影帽与历史子空间记录

```bash
g++ -O3 -std=c++17 src/kimi/p005/verify_projective_cap.cpp -o "$R/verify_projective_cap"
"$R/verify_projective_cap" data/kimi/p005/projective/pg10-cap2744.txt 11 2744
"$R/verify_projective_cap" data/kimi/p005/projective/pg11-cap6464.txt 12 6464
"$R/verify_projective_cap" data/kimi/p005/projective/pg12-cap13312.txt 13 13312

python3 src/kimi/p005/check_section_record.py \
  data/kimi/p005/projective/pg10-cap2744.txt \
  data/kimi/p005/projective/pg10-functionals.bin \
  results/kimi/p005/section-records/pg10-codim2-448.txt 11
python3 src/kimi/p005/check_section_record.py \
  data/kimi/p005/projective/pg11-cap6464.txt \
  data/kimi/p005/projective/pg11-functionals.bin \
  results/kimi/p005/section-records/pg11-codim3-472.txt 12
python3 src/kimi/p005/check_section_record.py \
  data/kimi/p005/projective/pg12-cap13312.txt \
  data/kimi/p005/projective/pg12-sample-functionals.bin \
  results/kimi/p005/section-records/pg12-codim4-496.txt 13
# 三条均应 PASS；这里直接复核历史记录，不需要重跑随机搜索。
```

大型关联掩码未入库，可用 `make_functionals.py` 和 `build_cap_masks.cpp` 重新生成。`codim2_search.cpp`、`codim3_search.cpp`、`codim4_search.cpp` 保留原始交替最佳响应实现；PG12 采样搜索编译时使用 `-DHVAL=131072`。这些是新的随机运行，不是上述历史记录复核的必要步骤。

### 10.7 全局非对称随机搜索（代表性新运行）

```bash
gcc -O3 -std=c11 src/kimi/p005/asym_anchors_target.c -o "$R/asym_anchors"
gcc -O3 -std=c11 src/kimi/p005/weighted2.c -o "$R/weighted2"

DBIAS=0 TARGET=497 "$R/asym_anchors" \
  1 1000000 data/kimi/p005/seeds496/seed0.txt "$R/asym" \
  0 8 1000000 17 0.12 3 1000000

DBIAS=0 FREEREMOVE=1 RESETW=1 "$R/weighted2" \
  1 1000000 513 data/kimi/p005/nearmiss/near10.txt "$R/weighted" \
  0 17 0.12 3 100 1000000 0
# 搜索程序以 0 表示找到合法目标，以 1 表示预算耗尽而未找到；
# 返回 1 不是数学上的不存在性结论。实际历史快照已归档，无需靠这些
# 新随机运行重现近 miss 后再做精确分析。
```


## 11. Kimi p005 第二轮（固定层 / 子空间切割 / 236 点构造）

继续从仓库根目录执行，并沿用第 10 节的 `PYTHONPATH` 和临时目录 `R`。

### 11.1 确定性生成并修复 236 点固定层近 miss

```bash
python3 src/kimi/p005/make_cap7_236.py "$R/cap236"
python3 src/kimi/p005/check_points.py \
  "$R/cap236/seed8-fixed236.txt" --expect-size 236 --expect-bad 0
# 预期：PASS cap7 size 236；嵌入层也是合法 236 点帽。

python3 src/kimi/p005/repair_nearmiss.py \
  data/kimi/p005/fixed236-near41.txt \
  --fixed data/kimi/p005/seed8-fixed236.txt \
  --prefix "$R/fixed236" --time 60
# 预期：status 0，conflict_lines 41，cover 41，dual 41，
# repaired 472，valid True，fixed_retained 236。
# 最优覆盖不唯一，复跑的具体 472 点可以与归档不同。
```

### 11.2 第 13 壳层的固定层联合重构

```bash
# 归档的 baseline-d13.txt 与平面切割就是本轮实际输入。
for c in {0..7}; do
  for v in 0 1 2; do
    python3 src/kimi/p005/shell_milp_cuts.py \
      results/kimi/p005/pools/baseline-d13.txt \
      data/kimi/p005/baseline512.txt \
      --remove 13 --minsize 512 --time 60 \
      --cuts results/kimi/p005/cuts/baseline-d13-plane-cuts.txt \
      --fix-coordinate "$c" --fix-value "$v" \
      --prefix "$R/fixed-${c}-${v}"
  done
done
# 24/24 预期 status 0 / Optimal，objective=dual_bound=512，valid True。
# 这只排除至少一个坐标层完全不变的改进，不是完整第 13 壳层证明。
```

### 11.3 三维和四维加强模型

```bash
python3 src/kimi/p005/check_cuts.py \
  results/kimi/p005/pools/baseline-d13.txt \
  data/kimi/p005/baseline512.txt \
  results/kimi/p005/cuts/baseline-d13-strong3-cuts.txt
python3 src/kimi/p005/check_cuts.py \
  results/kimi/p005/pools/baseline-d13.txt \
  data/kimi/p005/baseline512.txt \
  results/kimi/p005/cuts/baseline-d13-strong4-cuts.txt
# 两个文件都应 PASS；混合行格式为 bound n indices...。

python3 src/kimi/p005/shell_milp_cuts.py \
  results/kimi/p005/pools/baseline-d13.txt \
  data/kimi/p005/baseline512.txt \
  --remove 13 --minsize 512 --time 300 \
  --cuts results/kimi/p005/cuts/baseline-d13-strong3-cuts.txt \
  --prefix "$R/strong3"
# 原结果：时间限制，incumbent 512，dual_bound 514，未判定。
```

切割生成器 `plane_cuts.cpp`、`subspace3_cuts.cpp`、`subspace4_cuts.cpp` 均已归档；上述精确切割输入也已归档，因此复跑模型不依赖重新做随机或耗时的切割生成。求解器超时不是不可行性证明。


### 11.4 仿射副本并集重组

```bash
python3 src/kimi/p005/recombine_affine.py \
  data/kimi/p005/baseline512.txt "$R/recombine"
# 预期三个池分别为 975、982、1410 点；与 results/kimi/p005/pools/
# 下同名归档文件一致。每个仿射副本自身仍是合法 512 点帽。

python3 src/kimi/p005/shell_milp_cuts.py \
  results/kimi/p005/pools/recombine-k2-41001-pool.txt \
  data/kimi/p005/baseline512.txt \
  --remove 512 --minsize 512 --time 180 --prefix "$R/recombine-k2"
# 原记录：时间限制且没有 incumbent，未判定；不能读作 UNSAT。
```

普通搜索回到单一 512 点副本，强制混合锚集也未产生有效构造。上述命令用于复核精确输入和未判定状态，不声称仿射并集已经排除 513。


四维模型的实际命令与结果如下：

```bash
python3 src/kimi/p005/shell_milp_cuts.py \
  results/kimi/p005/pools/baseline-d13.txt data/kimi/p005/baseline512.txt \
  --remove 13 --minsize 512 --time 300 \
  --cuts results/kimi/p005/cuts/baseline-d13-strong4-cuts.txt \
  --prefix "$R/strong4"
# 合并模型：时间限制，无 incumbent，未判定。

python3 src/kimi/p005/shell_milp_cuts.py \
  results/kimi/p005/pools/baseline-d13.txt data/kimi/p005/baseline512.txt \
  --remove 13 --minsize 512 --time 300 \
  --cuts results/kimi/p005/cuts/baseline-d13-only4-cuts.txt \
  --prefix "$R/only4"
# 四维单独模型：时间限制，incumbent 512，dual_bound 528，未判定。
```



## 12. Kimi p005 第三轮（全空间 SAT 与单位见证正对照）

继续从仓库根目录执行，并沿用第 10 节的临时目录 `R`。DIMACS 文件约 160 MB／个，不提交到笔记仓库，可用生成器确定性重建。本次求解器为 Kissat 4.0.4，源码提交 `8af8e56f174b778aef3aa45af9f739b2a5f492c2`。

### 12.1 编译并复核两个规范化 512 点见证

```bash
git clone https://github.com/arminbiere/kissat.git "$R/kissat-src"
(
  cd "$R/kissat-src"
  git checkout 8af8e56f174b778aef3aa45af9f739b2a5f492c2
  sh configure
  make -j2
)
KISSAT="$R/kissat-src/build/kissat"
g++ -O3 -std=c++17 src/kimi/p005/gen_global_sat.cpp -o "$R/gen_global_sat"

python3 src/kimi/p005/check_points.py \
  data/kimi/p005/baseline512-normalized.txt --expect-size 512 --expect-bad 0
python3 src/kimi/p005/check_points.py \
  data/kimi/p005/baseline512-normalized-orbit12.txt --expect-size 512 --expect-bad 0
```

若挂载点不支持执行位或符号链接，把 Kissat 源码复制到本地临时文件系统后再执行 `sh configure` 和 `make`，并恢复 `configure`、`scripts/*.sh` 的执行权限。

### 12.2 无额外固定点的模型与直接编码正对照

```bash
"$R/gen_global_sat" "$R/global512-control.cnf" 512
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-control.cnf" "$R/global512-witness.cnf" \
  data/kimi/p005/baseline512-normalized.txt
"$KISSAT" --time=60 --sat "$R/global512-witness.cnf" \
  > "$R/global512-witness-kissat.log" 2>&1
# 求解器退出码 10 表示 SAT；归档日志在约 3.25 秒求解时间返回
# s SATISFIABLE。自由 512 和自由 513 历史运行均为 UNKNOWN，未判定。

"$R/gen_global_sat" "$R/global513.cnf" 513
"$KISSAT" --time=600 --sat "$R/global513.cnf" \
  > "$R/global513-kissat.log" 2>&1
# 这是一次昂贵的重新搜索，不是复现数学结论所必需；超时只能记 UNKNOWN。
```

无额外固定点的模型应有 7,173,360 条输入三点线、7,055,910 条化简后帽子句、327,679 个比较器；两个目标模型均为 523,689 个变量、8,607,295 条子句。

### 12.3 二支撑轨道的正对照和 513 分支

```bash
# 轨道 1：e1+e2，id 2916
"$R/gen_global_sat" "$R/global512-orbit11-control.cnf" 512 2916
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-orbit11-control.cnf" "$R/global512-orbit11-witness.cnf" \
  data/kimi/p005/baseline512-normalized.txt
"$KISSAT" --time=60 --sat "$R/global512-orbit11-witness.cnf" \
  > "$R/global512-orbit11-witness-kissat.log" 2>&1

# 轨道 2：e1+2e2，id 3645
"$R/gen_global_sat" "$R/global512-orbit12-control.cnf" 512 3645
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-orbit12-control.cnf" "$R/global512-orbit12-witness.cnf" \
  data/kimi/p005/baseline512-normalized-orbit12.txt
"$KISSAT" --time=60 --sat "$R/global512-orbit12-witness.cnf" \
  > "$R/global512-orbit12-witness-kissat.log" 2>&1
# 两个日志都应在约 3.36 秒求解时间返回 s SATISFIABLE。

"$R/gen_global_sat" "$R/global513-orbit11.cnf" 513 2916
"$R/gen_global_sat" "$R/global513-orbit12.cnf" 513 3645
"$KISSAT" --time=1200 --sat "$R/global513-orbit11.cnf" \
  > "$R/global513-orbit11-kissat.log" 2>&1
"$KISSAT" --time=1200 --sat "$R/global513-orbit12.cnf" \
  > "$R/global513-orbit12-kissat.log" 2>&1
```

两个轨道只覆盖“基点外至少有一个支撑 2 点”的规范化帽，不覆盖完全无二支撑外部点的情形。

### 12.4 若日志出现 SAT，回读并独立检查

```bash
python3 src/kimi/p005/extract_sat_cap.py \
  "$R/global513-orbit11-kissat.log" "$R/candidate513.txt" \
  --extra 2916 --min-size 513
# 第二轨道改用 --extra 3645。脚本会补回常量基点，并逐点对检查帽集合法性。
# 之后仍须转换为官方 certificate.json，并由官方 tools/check.py 返回 0。
```

实际历史状态以 `logs/kimi/p005/global51*-kissat.log` 的 `s` 行为准。`UNKNOWN`、外部终止和超时都不是 UNSAT；没有 SAT 候选时不得创建 Shot 或写入突破分数。


### 12.5 坐标平面加强和无二支撑第三分区

生成器第四个可选参数 `coordinate_planes` 控制是否加入全部 20,412 个坐标平面；第五个参数 `forbid_support2` 控制是否禁用全部支撑 2 点。不需要额外固定点时将第三个参数写为 `-1`。

```bash
# 重放无二支撑基点的确定性随机搜索；NumPy 即可，使用单线程 BLAS。
OPENBLAS_NUM_THREADS=1 python3 src/kimi/p005/search_basis_low_support.py \
  data/kimi/p005/baseline512-normalized.txt "$R/basis-low" \
  --trials 2000 --seed 20261030
# 预期最后一条 NEW_BEST 在 trial=1980 得到 weight2=0。
cmp "$R/basis-low-w2-0.txt" data/kimi/p005/baseline512-normalized-no-support2.txt
python3 src/kimi/p005/check_points.py \
  data/kimi/p005/baseline512-normalized-no-support2.txt --expect-size 512 --expect-bad 0

# 二支撑轨道的平面加强正对照。
"$R/gen_global_sat" "$R/global512-orbit11-planes-control.cnf" 512 2916 1
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-orbit11-planes-control.cnf" \
  "$R/global512-orbit11-planes-witness.cnf" \
  data/kimi/p005/baseline512-normalized.txt
"$KISSAT" --time=60 --sat "$R/global512-orbit11-planes-witness.cnf" \
  > "$R/global512-orbit11-planes-witness-kissat.log" 2>&1
# 预期：PLANES forms 20412 clauses 2498805；求解 s SATISFIABLE。

# 无二支撑分区的平面加强正对照。
"$R/gen_global_sat" "$R/global512-no-support2-planes-control.cnf" 512 -1 1 1
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-no-support2-planes-control.cnf" \
  "$R/global512-no-support2-planes-witness.cnf" \
  data/kimi/p005/baseline512-normalized-no-support2.txt
"$KISSAT" --time=60 --sat "$R/global512-no-support2-planes-witness.cnf" \
  > "$R/global512-no-support2-planes-witness-kissat.log" 2>&1
# 预期：FORBID_SUPPORT2 total 112 newly 84；
# PLANES forms 20412 clauses 2408616；求解 s SATISFIABLE。

# 对应的 513 搜索。
"$R/gen_global_sat" "$R/global513-orbit11-planes.cnf" 513 2916 1
"$R/gen_global_sat" "$R/global513-no-support2-planes.cnf" 513 -1 1 1
"$KISSAT" --time=1200 --sat "$R/global513-orbit11-planes.cnf" \
  > "$R/global513-orbit11-planes-kissat.log" 2>&1
"$KISSAT" --time=1200 --sat "$R/global513-no-support2-planes.cnf" \
  > "$R/global513-no-support2-planes-kissat.log" 2>&1
```

无二支撑模型的统计应为 6,786,368 条帽子句、2,408,616 条平面子句、517,083 个变量和 10,726,551 条总子句。三个分区合在一起覆盖全部基点规范化帽，但只有求解器明确返回 UNSAT 才能排除对应分区；运行中、超时和外部终止均不作此解释。


### 12.6 第二轨道平面化与全一型三支撑细化

```bash
# 第二二支撑轨道的平面正对照。
"$R/gen_global_sat" "$R/global512-orbit12-planes-control.cnf" 512 3645 1
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-orbit12-planes-control.cnf" \
  "$R/global512-orbit12-planes-witness.cnf" \
  data/kimi/p005/baseline512-normalized-orbit12.txt
"$KISSAT" --time=60 --sat "$R/global512-orbit12-planes-witness.cnf" \
  > "$R/global512-orbit12-planes-witness-kissat.log" 2>&1
# 预期：PLANES clauses 2497695；求解 s SATISFIABLE。

# 自动选择具有相同 0/1/2 坐标计数的源点，并置换成 id 3159。
python3 src/kimi/p005/permute_cap_point.py \
  data/kimi/p005/baseline512-normalized-no-support2.txt 3159 \
  "$R/baseline512-no-support2-weight3-t0.txt"
cmp "$R/baseline512-no-support2-weight3-t0.txt" \
  data/kimi/p005/baseline512-no-support2-weight3-t0.txt

"$R/gen_global_sat" "$R/global512-no2-weight3-t0-control.cnf" 512 3159 1 1
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-no2-weight3-t0-control.cnf" \
  "$R/global512-no2-weight3-t0-witness.cnf" \
  data/kimi/p005/baseline512-no-support2-weight3-t0.txt
"$KISSAT" --time=60 --sat "$R/global512-no2-weight3-t0-witness.cnf" \
  > "$R/global512-no2-weight3-t0-witness-kissat.log" 2>&1
# 预期：CAP emitted 6757928；PLANES clauses 2395738；
# vars 516401；clauses 10683187；求解 s SATISFIABLE。

"$R/gen_global_sat" "$R/global513-no2-weight3-t0.cnf" 513 3159 1 1
"$KISSAT" --time=1200 --sat "$R/global513-no2-weight3-t0.cnf" \
  > "$R/global513-no2-weight3-t0-kissat.log" 2>&1
```

细化分支只优先探索无二支撑总分区的一部分。若回读该模型的 SAT 解，`extract_sat_cap.py` 应使用 `--extra 3159`；仍然必须通过独立帽集检查和官方 verifier。


### 12.7 大 CNF 的压缩持久化与流式求解

直接在持久挂载写大于约 200 MiB 的 CNF 曾在本次环境中造成截断。不要只信生成器的 `DONE`：必须同时检查头部、实际行数和解压后的完整性。

```bash
# 以轨道 2916 为例；在本地临时文件系统生成。
"$R/gen_global_sat" "$R/global513-orbit11-planes.cnf" 513 2916 1
head -n 1 "$R/global513-orbit11-planes.cnf"
test "$(wc -l < "$R/global513-orbit11-planes.cnf")" -eq 11078225
gzip -1 -c "$R/global513-orbit11-planes.cnf" \
  > "$R/global513-orbit11-planes.cnf.gz"
gzip -t "$R/global513-orbit11-planes.cnf.gz"
test "$(gzip -dc "$R/global513-orbit11-planes.cnf.gz" | wc -l)" -eq 11078225

# 流式求解；第五个参数是本地 Kissat 可执行文件。
TIME=1200 bash src/kimi/p005/run_compressed_sat.sh \
  "$R/global513-orbit11-planes.cnf.gz" \
  "$R/stream-orbit11-kissat.log" "$R/stream-orbit11.rc" 3 "$KISSAT"
```

另外两个模型的正确解压行数为：轨道 3645 平面模型 11,077,133 行；无二支撑且固定 3159 的细化模型 10,683,188 行。压缩包的实际 SHA-256 见 `logs/kimi/p005/cnf-gzip-integrity.log`。

运行器用 `pipefail` 同时捕捉 gzip 和求解器错误。退出码 10／20 才是 Kissat 的 SAT／UNSAT；0 通常为 UNKNOWN，1 或 2 需先检查解析、解压和调用错误。若二进制位于不支持执行位的挂载点，可设置 `LOADER=/lib64/ld-linux-x86-64.so.2`，第五个参数仍传 Kissat 路径。


### 12.8 第二类三支撑分支

```bash
python3 src/kimi/p005/permute_cap_point.py \
  data/kimi/p005/baseline512-normalized-no-support2.txt 3402 \
  "$R/baseline512-no-support2-weight3-t1.txt"
cmp "$R/baseline512-no-support2-weight3-t1.txt" \
  data/kimi/p005/baseline512-no-support2-weight3-t1.txt

"$R/gen_global_sat" "$R/global512-no2-weight3-t1-control.cnf" 512 3402 1 1
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-no2-weight3-t1-control.cnf" \
  "$R/global512-no2-weight3-t1-witness.cnf" \
  data/kimi/p005/baseline512-no-support2-weight3-t1.txt
"$KISSAT" --time=60 --sat "$R/global512-no2-weight3-t1-witness.cnf" \
  > "$R/global512-no2-weight3-t1-witness-kissat.log" 2>&1
# 预期：vars 516433；clauses 10687515；s SATISFIABLE。

"$R/gen_global_sat" "$R/global513-no2-weight3-t1.cnf" 513 3402 1 1
test "$(wc -l < "$R/global513-no2-weight3-t1.cnf")" -eq 10687516
gzip -1 -c "$R/global513-no2-weight3-t1.cnf" \
  > "$R/global513-no2-weight3-t1.cnf.gz"
TIME=1200 bash src/kimi/p005/run_compressed_sat.sh \
  "$R/global513-no2-weight3-t1.cnf.gz" "$R/stream-t1-kissat.log" \
  "$R/stream-t1.rc" 0 "$KISSAT"
```

退出码 137 表示 SIGKILL，不是 UNSAT；资源受限环境下最多并发两个此类大模型。T1 回读 SAT 解时使用 `--extra 3402`。


### 12.9 固定 192 点层、放开另外两层的 SAT 模型

该模型是原 512 基线的受限分支，不是全空间完备分区。它固定第一坐标为 0 的 192 个基线点，允许另外两层同时改变；与固定两层后优化单层不同。

```bash
g++ -O3 -std=c++17 src/kimi/p005/gen_fixed_layer_sat.cpp \
  -o "$R/gen_fixed_layer_sat"

# 直接生成带原基线单位见证的 512 正对照。
"$R/gen_fixed_layer_sat" "$R/fixed192-512-witness.cnf" \
  data/kimi/p005/baseline512.txt 0 0 512 \
  "$R/fixed192.map" data/kimi/p005/baseline512.txt
# 应有：192 个固定点、4374 个原始自由变量、1593594 条三元帽子句、
# 419904 条跨层二元子句；加见证后共 3037567 条子句。
"$KISSAT" --time=60 --sat "$R/fixed192-512-witness.cnf" \
  > "$R/fixed192-512-witness-kissat.log" 2>&1
# 求解器退出码 10 表示 SAT；归档日志在 1.13 秒求解时间返回 SAT。

python3 src/kimi/p005/extract_fixed_layer_sat.py \
  "$R/fixed192-512-witness-kissat.log" "$R/fixed192.map" \
  data/kimi/p005/baseline512.txt 0 0 "$R/fixed192-recovered512.txt" \
  --min-size 512
sort -n data/kimi/p005/baseline512.txt > "$R/baseline512-sorted.txt"
cmp "$R/fixed192-recovered512.txt" "$R/baseline512-sorted.txt"

# 生成不带见证的 513 主模型：固定 192 点，自由层目标为 321 点。
"$R/gen_fixed_layer_sat" "$R/fixed192-513.cnf" \
  data/kimi/p005/baseline512.txt 0 0 513 "$R/fixed192.map"
test "$(wc -l < "$R/fixed192-513.cnf")" -eq 3033194
gzip -1 -c "$R/fixed192-513.cnf" > "$R/fixed192-513.cnf.gz"
gzip -t "$R/fixed192-513.cnf.gz"
test "$(gzip -dc "$R/fixed192-513.cnf.gz" | wc -l)" -eq 3033194
TIME=1200 bash src/kimi/p005/run_compressed_sat.sh \
  "$R/fixed192-513.cnf.gz" "$R/stream-fixed192-kissat.log" \
  "$R/stream-fixed192.rc" 0 "$KISSAT"
```

若主模型出现 SAT，使用同一映射回读并独立验证，再进入官方证书流程：

```bash
python3 src/kimi/p005/extract_fixed_layer_sat.py \
  "$R/stream-fixed192-kissat.log" "$R/fixed192.map" \
  data/kimi/p005/baseline512.txt 0 0 "$R/fixed192-candidate513.txt" \
  --min-size 513
```

`UNKNOWN`、外部杀死或超时均不表示这个固定层分支不可行，更不表示全空间不存在 513 点帽。

### 12.10 其余两类三支撑见证

```bash
python3 src/kimi/p005/permute_cap_point.py \
  data/kimi/p005/baseline512-normalized-no-support2.txt 4131 \
  "$R/baseline512-no-support2-weight3-t2.txt"
python3 src/kimi/p005/permute_cap_point.py \
  data/kimi/p005/baseline512-normalized-no-support2.txt 6318 \
  "$R/baseline512-no-support2-weight3-t3.txt"
cmp "$R/baseline512-no-support2-weight3-t2.txt" \
  data/kimi/p005/baseline512-no-support2-weight3-t2.txt
cmp "$R/baseline512-no-support2-weight3-t3.txt" \
  data/kimi/p005/baseline512-no-support2-weight3-t3.txt
python3 src/kimi/p005/check_points.py \
  data/kimi/p005/baseline512-no-support2-weight3-t2.txt --expect-size 512 --expect-bad 0
python3 src/kimi/p005/check_points.py \
  data/kimi/p005/baseline512-no-support2-weight3-t3.txt --expect-size 512 --expect-bad 0
```

对应的全局生成器参数分别为 `513 4131 1 1` 和 `513 6318 1 1`；若求得 SAT，回读时分别使用 `--extra 4131`、`--extra 6318`。即使四个三支撑分支全部 UNSAT，也只排除“无二支撑但至少有一个三支撑点”的情形；不覆盖同时无二、三支撑点的情形，也不改变两个二支撑分区各自的状态。


### 12.11 T2／T3 正对照与固定层对称破缺

T2、T3 的完整正对照流程如下（以 T2 为例；T3 把 `t2`、`4131` 分别换成 `t3`、`6318`）：

```bash
"$R/gen_global_sat" "$R/global512-no2-weight3-t2-control.cnf" \
  512 4131 1 1
python3 src/kimi/p005/add_sat_witness.py \
  "$R/global512-no2-weight3-t2-control.cnf" \
  "$R/global512-no2-weight3-t2-witness.cnf" \
  data/kimi/p005/baseline512-no-support2-weight3-t2.txt
"$KISSAT" --time=60 --sat "$R/global512-no2-weight3-t2-witness.cnf" \
  > "$R/global512-no2-weight3-t2-witness-kissat.log" 2>&1
python3 src/kimi/p005/extract_sat_cap.py \
  "$R/global512-no2-weight3-t2-witness-kissat.log" "$R/t2-recovered512.txt" \
  --extra 4131 --min-size 512
sort -n data/kimi/p005/baseline512-no-support2-weight3-t2.txt \
  > "$R/t2-cap-sorted.txt"
cmp "$R/t2-recovered512.txt" "$R/t2-cap-sorted.txt"
# 归档 T2 正对照在 4.07 秒返回 SAT；T3 也为 4.07 秒。

"$R/gen_global_sat" "$R/global513-no2-weight3-t2.cnf" 513 4131 1 1
test "$(wc -l < "$R/global513-no2-weight3-t2.cnf")" -eq 10691988
# T3 主模型应为 10696289 行。
```

固定层生成器的最后三个可选参数为 `map.txt|-`、`witness_cap.txt|-`、`balance_free_layers`。最后的平衡参数利用两自由层互换对称性；512 对照要求第一自由层至少 160 点，513 主模型要求至少 161 点。

```bash
"$R/gen_fixed_layer_sat" "$R/fixed192-balanced512-witness.cnf" \
  data/kimi/p005/baseline512.txt 0 0 512 \
  "$R/fixed192-balanced.map" data/kimi/p005/baseline512.txt 1
"$KISSAT" --time=60 --sat "$R/fixed192-balanced512-witness.cnf" \
  > "$R/fixed192-balanced512-witness-kissat.log" 2>&1
# 应有 vars 487984、clauses 3468704（含单位见证）；
# 归档日志在 1.52 秒返回 SAT。

"$R/gen_fixed_layer_sat" "$R/fixed192-balanced513.cnf" \
  data/kimi/p005/baseline512.txt 0 0 513 \
  "$R/fixed192-balanced.map" - 1
test "$(wc -l < "$R/fixed192-balanced513.cnf")" -eq 3464331
gzip -1 -c "$R/fixed192-balanced513.cnf" \
  > "$R/fixed192-balanced513.cnf.gz"
gzip -t "$R/fixed192-balanced513.cnf.gz"
TIME=1200 bash src/kimi/p005/run_compressed_sat.sh \
  "$R/fixed192-balanced513.cnf.gz" "$R/stream-fixed192-balanced-kissat.log" \
  "$R/stream-fixed192-balanced.rc" 0 "$KISSAT"
```

该对称破缺只固定层分支内有效：坐标值缩放固定第 0 层并互换两个自由层，因此没有遗漏该分支的仿射等价解。它不能被直接套用到其他全局基点模型。


### 12.12 固定层动态加权候选生成器

先确定性重建三类输入，并与归档文件比较：

```bash
python3 src/kimi/p005/make_fixed_layer_weighted.py \
  data/kimi/p005/baseline512.txt 0 0 "$R/fixed192"
cmp "$R/fixed192-base.txt" data/kimi/p005/fixed192-base.txt
cmp "$R/fixed192-banned.txt" data/kimi/p005/fixed192-banned.txt
cmp "$R/fixed192-initial513.txt" data/kimi/p005/fixed192-initial513.txt
python3 src/kimi/p005/check_points.py \
  "$R/fixed192-initial513.txt" --expect-size 513 --expect-bad 8
gcc -O3 -std=c11 src/kimi/p005/weighted_fixedbase.c \
  -o "$R/weighted_fixedbase"
```

保护点机制的合法 512 控制：

```bash
PROTECTFILE="$R/fixed192-base.txt" DBIAS=0 FIXBASE=0 \
"$R/weighted_fixedbase" \
  0 1000 512 data/kimi/p005/baseline512.txt "$R/fixed192-control" \
  0 7 0.15 3 1000 100000 0 "$R/fixed192-banned.txt" \
  > "$R/fixed192-weighted-control.log" 2>&1
python3 src/kimi/p005/check_points.py \
  "$R/fixed192-control-wvalid-0.txt" --expect-size 512 --expect-bad 0
```

程序对任何合法目标都会打印通用的 `BREAKTHROUGH` 字样；这里的 512 控制不是纪录突破。只有独立检查和官方 verifier 才能确认提交资格。

513 主搜索可从下列命令复放；它是启发式候选生成，不是排除性证明，运行预算可调整：

```bash
PROTECTFILE="$R/fixed192-base.txt" DBIAS=0 FIXBASE=0 RESETW=1 \
"$R/weighted_fixedbase" \
  20261030 20000000 513 "$R/fixed192-initial513.txt" "$R/fixed192-weighted" \
  0 7 0.15 3 1000 3000000 0 "$R/fixed192-banned.txt" \
  > "$R/fixed192-weighted.log" 2>&1
# 若生成 fixed192-weighted-wvalid-0.txt：
python3 src/kimi/p005/check_points.py \
  "$R/fixed192-weighted-wvalid-0.txt" --expect-size 513 --expect-bad 0
```

`PROTECTFILE` 的每个点都必须属于初始点文件；禁用文件覆盖固定层的全部非基线点。主搜索若只降低冲突数而未达到 0，不得把近 miss 当作合法帽。
