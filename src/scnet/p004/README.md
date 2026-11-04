# p004 源码（2026-10-31 轮次）

C7 香农容量：C7<sup>⊠10</sup> 的独立集。本目录是这一轮全部搜索代码，按“可复核”
优先排序。所有结论都由官方 verifier 或显式的完备性标志给出；没有一处把
`UNKNOWN` 当成 `UNSAT`。

## 文件

| 文件 | 作用 | 依赖 |
| --- | --- | --- |
| `R367.txt` | 论文构造用到的 367 点 F₇⁵ 独立集（5 列，空格分隔） | — |
| `model.py` | 审计后的配对条带模型：`StripModel`、`AsymmetricStripModel`、2-SAT、官方校验；含 `selftest()` | 仅标准库 |
| `paper_construct.py` | 逐字复现 134,753 点论文构造；`--compare` 与原始文件比对 | 仅标准库 |
| `frontier9.py` | 九对族的最小删点优化（CP-SAT），复现“17 空位 → 134,753” | OR-Tools |
| `sweep.py` | 九对候选（50 个）、R1/R2 变体、自同构扫描、非对称补丁搜索；**含检查点：243 个状态、frontier=0（该族已闭合）** | OR-Tools |
| `cand9.py` | 重新枚举第 9 对的 50 个候选（pool(D8) 计算） | 仅标准库 + `model.py` |
| `j0_scan2.py` | 56 种 `J0` 全扫描（3-子集 × 8 对），只有 `{0,5,6}` 给出独立集 | 仅标准库 |
| `root_scan.py` | 50 个“第 9 对候选”逐个建族求根；全部在结构检查处被拒 | OR-Tools |
| `xk.c` | C7⁵ 上对 367 点集做完备的 k-交换检查（删 k 加 k+1），k ≤ 6 全无 | g++ ≥ 11 |
| `xk_par.c` | 同上，按首点并行的版本（`std::thread`，无需 OpenMP）；节点数与 `xk.c` 逐位一致 | g++ ≥ 11 |
| `xk10.cpp` | **增量计数器版**：`conflictCnt` + `freeB` 位图，节点里不再扫描活跃表；k=9 204.1 s（xk7 的 6.2×），k=11 2495.6 s（1.8×）；k=13 外推 ~39 h，配额内不可达 | g++ ≥ 11 |
| `xk8.cpp` | `xk7` 的位掩码/位图/活跃前沿版；自测计数与 `xk7` 完全一致，k≤9 快 2–7×，k=12 反而慢 3% | g++ ≥ 11 |
| `xk9.cpp` | 在 `xk8` 上把 touch 与并集大小做成 O(1)（`touchCnt`/`popU`）；k=9 单线程 287.9 s（xk7 的 4.4×），k=12 无收益 | g++ ≥ 11 |
| `xk7.cpp` | **连通枚举**：规范序生成每个连通集恰好一次，把 k=9/10 从“≥1.6×10^10 节点”降到分钟级；`--selftest N m0 mrange kmax` 用暴力对拍 | g++ ≥ 11 |
| `d9_scan.py` | 论文 8 对 + 50 个第 9 对候选 × 29 种 `J0` = 1,450 个 134,770 构造，全部非独立 | 仅标准库 |
| `emit_class.py` | 把检查点里的某一类还原成显式 134,753 点集并跑官方 verifier | OR-Tools |
| `sixm3.c` | 审计重写的 k→k+1 交换排除（C++17，OpenMP），自检 + 完备性标志；`--deep6` 打开 rank-4 锚点层，`--deep3` 打开 rank-3 锚点层，`--conn6` 打开连通 6-集，`--shard i/n` 分片覆盖 | g++ ≥ 11 |
| `lns4.c` | 平台/中性重排搜索（启发式，非穷举），需与 `sixm3.c` 拼成一个编译单元 | g++ ≥ 11 |

## 快速校验（不需要 OR-Tools）

```bash
export FEW_SHOTS_REPO=/path/to/few-shots
cd src/scnet/p004

python3 model.py
# SELFTEST adjacency=10049 random_2SAT=300 direct_extension_cases=160 PASS
# CONTROL pairs=8 X=367 active_X=46 contact_signatures=22 variables=352 clauses=916 reconstructed_points=134753

python3 paper_construct.py --compare /path/to/CC_10_7_134753.txt
# SIZE 134753 B 359 A 20 D 26
# OFFICIAL_VERIFY {"valid": true, "score": "134753", "canonical_hash": "650b0556...", "message": "ok"}
# COMPARE {"reference": 134753, "mine": 134753, "only_reference": 0, "only_mine": 0}
```

`J0={0,5,6}` 不是任意选的：`python3 j0_scan2.py` 把 8 个私有对里的 3-子集全部试一遍
（56 种），**只有 `{0,5,6}` 给出独立集**（134,753 点），其余 55 种都在跨前缀冲突处失败。

`paper_construct.py` 的 `J0={0,5,6}`、`J1={1,2,3,4,7}` 必须**包含 pair 7**。
把它误写成 `J1={1,2,3,4}` 时集合大小仍是 134,753，但不是独立集（与正确集合的
对称差 340 点，170/170），`is_valid` 会直接报错——这正是本轮早期一次 176 点
偏差的来源。这个反例现在由自检兜住。

## 九对族的前沿（需要 OR-Tools）

```bash
python3 frontier9.py --variant R2 --x patch --seconds 60
# FRONTIER9 {"variant": "R2", "x": "patch", "status": "OPTIMAL", "active_slots": 900, "raw_size": 134770}
# FRONTIER9_BOUND {"omissions": 17, "proven_optimal": true, "size": 134753}
# FRONTIER9_VERIFY {"valid": true, "score": "134753", "canonical_hash": "1e6929...", "message": "ok"}
```

六个固定 X 的最优删点数（全部 `OPTIMAL`，本轮复算）：

| 变体 | X | 最少删点 | 规模 | 备注 |
| --- | --- | ---: | ---: | --- |
| R2 | `tr(R)+patch` | **17** | 134,753 | 距 134,754 只差 1 个空位 |
| R1 | `patch` / `tr(R)` / `X0` | 18 | 134,752 | — |
| R2 | `X0` / `tr(R)` | 32 | 134,738 | — |

`17` 只对“这六个固定 (pairs, X)”成立，不是对全部九对族、更不是对全部构造的结论。

## 交换排除（C++）

```bash
g++ -x c++ -std=c++17 -O3 -fopenmp sixm3.c -o sixm3

./sixm3 --self-test
# SELFTEST coordinate_cases=49 random_graphs=1000 targets_per_graph=6 forced_hash_collisions=100 PASS
# MASK_SELFTEST random_pairs=100000 PASS baseline=134753

./sixm3 --input ../../../data/scnet/p004/cc_10_7_134753_paper.txt --max-k 1 --threads 8 --seconds 300
# COUNTS m0=0 m1=5178 m2=144861 m3=999144 m4=2131704 m5=2550704 m6=3197204 blocker_mismatches=0 truncations=0
# STAGE natural_k1 sets_or_anchor_instances=5152 eligible=26 max_pool=2 complete=1 found=0
# COMPLETE exchanges_up_to=1 no_improvement=1
```

`lns4.c` 的构建方式（`main` 改名后与 `sixm3.c` 拼成一个翻译单元）：

```bash
sed 's/^int main(int argc,char\*\* argv){/int audited_baseline_main(int argc,char** argv){/' sixm3.c > /tmp/core.cpp
cat /tmp/core.cpp lns4.c > /tmp/lns4_all.cpp
g++ -x c++ -std=c++17 -O3 -fopenmp /tmp/lns4_all.cpp -o lns4

./lns4 --self-test
# PLATEAU_SELFTEST graphs=500 targets=9 node_limit_reporting=PASS
./lns4 --input ../../../data/scnet/p004/cc_10_7_134753_paper.txt --chains 8 --seconds 120 --radius 1024
```

在 macOS 上没有 libomp 时，可以用一个声明 `omp_*` 的临时桩文件串行编译，用于
只验证逻辑/自检；性能数字必须在 Linux + `-fopenmp` 上重跑。

## 连通枚举（xk7 / xk8 / xk9）

三个实现枚举同一批对象（连通 k+1 集，`|T(A)| ≤ k`），自测计数逐例相同。
`xk7` 最慢但最省内存；`xk8`/`xk9` 在 k ≤ 9 上快 2–7×；**k = 12 上三者同速**
（`xk9` 22475 s vs `xk7` 21724 s），因为深层几乎所有候选都“活跃”，前沿优化失效。
结论：想要 k = 13，需要的是结构性剪枝（例如 13 个点 + ≤13 个阻塞点的关联图是树/单圈图），
不是这些常数优化。

### 连通枚举（xk10，当前最快）

```bash
g++ -x c++ -std=c++17 -O3 xk10.cpp -o xk10 -pthread
./xk10 --selftest 2000 8 7 6        # 与暴力对拍
./xk10 11 15 R367.txt               # K=11, 15 线程
```

节点内维护 `freeB = {v : touchCnt>0, conflictCnt==0, 不在 A}`，按位遍历；
并集大小用 `popU + |T(v)| - touchCnt[v]` 一次性判定。自测计数与 `xk7/xk8/xk9` 完全一致。
实测：k=9 204.1 s（xk7 1267.8 s）；k=11 2495.6 s（xk7 4420 s）。
**k=12 每步节点数增长 6.80×**，故 k=13 ≈ 1.2e11 节点 / 1.7e14 次扫描 ≈ 39 h——常数优化已到头，
要再往下必须减少节点（数学层面的归约），不是把内层写得更快。

### 连通枚举（xk7）

```bash
g++ -x c++ -std=c++17 -O3 xk7.cpp -o xk7 -pthread

# 暴力 vs 1 线程 vs 4 线程，同一 seed；三个配置合计 5,300 个实例
./xk7 --selftest 3000 7 6 3
# SELFTEST instances=3000 mismatches=0 positive=1842 total_connected_sets=11316 max_per_instance=112
./xk7 --selftest 2000 8 7 6
# SELFTEST instances=2000 mismatches=0 positive=1122 total_connected_sets=12824 max_per_instance=245
./xk7 --selftest 300 10 9 7
# SELFTEST instances=300 mismatches=0 positive=197 total_connected_sets=6689 max_per_instance=440

# 真实实例：需要 R367 为 367 行的 C7^5 点表（5 个 0..6 的十进制数一行）
./xk7 9 15 R367.txt
# K=9 m=16377 threads=15 connected=1
# CONNECTED_SETS size=10 union<=9 count=0 seconds=208.4
```

`xk7` **不检查**“更小的 k 已排除”。它只枚举连通见证，完备性靠定理：
若某个见证不连通，它的一个连通分量就是更小的改进交换。所以跑 `K` 之前必须先
穷尽 `0..K-1`（本项目里 k ≤ 8 由 `xk.c`/`xk_par.c` 给出，k=0 由直接检查给出）。

## 结论边界

- `sixm3` 的 k ≤ 5 完备性在**两个** 134,753 点集上成立（论文集、rearranged 集），
  都 `found=0`、`truncations=0`。这是“这两个集合没有 k→k+1 交换”，不是全局下界。
- k=6 跑了自然 6-集、rank-5 单扩展、rank-4 锚点层（64 个分片、6.44 亿实例）、
  rank-3 锚点层（64 个分片、17.5 亿实例）和连通 6-集（3,759 万个去重集合），
  全部 `complete=1 found=0`。这五个阶段仍不等于“所有 6-集”，k=6 的覆盖面按阶段陈述。
- `xk7` 的 k=9/10 完备性只到“R367 的局部邻域”：它给不出 `α(C7⁵)=367`，
  也不排除其他 367 点集旁边的 368 点集。
- 旧目录里的 `quad45.c/tri45b.c/five6.c/five6b.c/pair5.c` 有 15 位键打包溢出、
  C 负数取模的邻接错误、候选索引错配、容量静默截断和 P5/K1,4 覆盖缺口。
  它们此前的 k≤5“完备”结论已作废；本目录的 `sixm3.c` 是替代实现。
