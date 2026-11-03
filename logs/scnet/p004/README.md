# p004 运行日志

## `sixm3_k6_600s.log`

2026-11-01，本机串行（macOS 无 libomp，用 `/tmp/ompstub` 的 OpenMP 桩编译，
实际 1 线程），`--max-k 6 --threads 8 --seconds 600`，输入是 arXiv/论文构造的
134,753 点集（`data/scnet/p004/cc_10_7_134753_paper.txt`）。

```bash
g++ -x c++ -std=c++17 -O3 -I/tmp/ompstub sixm3.c /tmp/ompstub/omp.c -o sixm3
./sixm3 --self-test
./sixm3 --input ../../../data/scnet/p004/cc_10_7_134753_paper.txt --max-k 6 --threads 8 --seconds 600
```

### 这份日志能证明什么

- **k ≤ 5 全部完成**：`natural_k1..5`、`rank3_extensions_k4`、`rank4/rank3_extensions_k5`、
  `connected_pair_k3..k5` 每条都是 `complete=1 found=0`，且
  `truncations=0`、`blocker_mismatches=0`（自检里）。
  其中 k=5 的 rank3 阶段处理了 **181,817,650** 个实例，连通对阶段生成
  17,040,783 个 5-集（去重后 6,511,209）。
  这是第一次在本地把此前只有“远端记录”的 k ≤ 5 结论完整复现出来。
- **k=6 的两个设计内阶段完成**：`natural_k6`（3,003,919 个 6-集）与
  `rank5_extensions_k6`（41,986,400 个实例），均 `found=0`。

### 这份日志不能证明什么

- `RESULT {"status":"completed","max_k_completed":5,...}` 里的 `completed`
  只表示“程序把它为 k=6 设计的两阶段跑完”，**不是** k=6 全覆盖。
  源码里 `if(k==6){anchorStage(5,6);break;}` 让 `max_k_completed` 永远停在 5；
  深层 rank4/rank3 扩展和连通 6-集根本没有实现。
- 因此不能说“k ≤ 6 已排除”，只能说“k ≤ 5 排除；k=6 部分覆盖，未发现改进”。
- 这是一次运行的日志，不是证明 134,754 不存在；它只覆盖这一个点集和这些邻域。

### 复现提示

- `--seconds` 上限是 600（源码里的 `require`），想要更长只能改代码或分段。
- 串行 600 秒刚好跑完上述阶段；若不编译 OpenMP（真并行），时间会富裕很多，
  但**没有** k=6 深层的实现，跑再久也不会自动扩展到那里。

## `sixm3_rank4_k6_all64.log`

2026-11-01，`--max-k 6 --only-deep6 --shard i/64 --threads 1 --seconds 300`，
`i = 0…63`，串行依次运行。每个 `STAGE rank4_extensions_k6 … complete=1 … shard=i/64`
是一个分片的完整结果，紧随其后的 `SKIP rank3_extensions_k6` 是固定输出（见下）。

- **64 个分片全部 `complete=1`，`found=0`，无 `ERROR`**；
- 合计 **644,208,231** 个实例（每个分片约 1,000 万，最多 1,008 万）；
- 覆盖 `rank4_extensions_k6` 的 **64/64**（这一阶段自己定义的那一族，见下）。

### 这一阶段枚举的是什么

`extensions(a,rank,k)` 把锚点 `a` 与 `incidence` 中**和 `a` 有公共点**的 blocker 记录 `b`
求并集，`merge()` 只保留并集不超过 `k` 个点的结果。所以这里的实例形如

```
a ∪ b,  a 是 4 点 blocker,  b 是 2/3/4 点 blocker,  |a∩b| ≥ 1,  |a∪b| = 6
```

**不是**“所有包含 4 点 blocker 的 6-集”：若另外两个点既不构成 blocker、也不与任何
blocker 相接，这样的 6-集不在这一阶段。正确说法只能是
“该阶段定义的 6.44 亿个 6-集全部检查过，没有改进”。

### 分片的含义

`--shard i/n` 只作用于按 head 索引的循环（`naturalStage`、`anchorStage`），
把下标限制为 `i mod n`。`connectedPairs` 的生成不参与分片，所以**不要**把分片的
`connected_pair_*` 行相加（每个分片都会重跑全量）。这份日志用了 `--only-deep6`，
因此没有 k≤5 的重复行。

### `SKIP` 行

`anchorStage(3,6)`（rank-3 锚点、深度 3 扩展）**没有实现**：`extensions()` 只支持
深度 ≤ 2。程序打印

```
SKIP rank3_extensions_k6 reason=extension_depth_3_not_implemented
```

后正常结束并输出 `RESULT`。早期版本在这里会 `ERROR unsupported extension depth`
并中断，那 16 个分片的日志已经删除，以这份为准。

### 仍然没覆盖的部分

- `natural_k6` 与 `rank5_extensions_k6`：见 `sixm3_k6_600s.log`（已跑，`found=0`）；
- rank-3 锚点的深度 3 扩展：未实现（`extensions()` 只到深度 2）。抽样估计这一层约
  27 亿个实例、单进程约 3 小时（1/256 分片 42.5 秒 / 837 万实例），本轮没有跑。本地试做的 `--deep3`
  变体剪枝前提是错的，会漏集合，**没有归档，也不算覆盖**；
- 连通 6-集（`pairStage(6)`，源码里没有实现）。

所以完整表述是：“k ≤ 5 排除；k=6 覆盖了自然 6-集、rank-5 扩展和
`rank4_extensions_k6` 定义的那一族 6-集，均未发现改进；其余 6-集未覆盖”。

## `d9_scan_2026-11-01.log`

`src/scnet/p004/d9_scan.py` 的输出（2026-11-01，约 45 秒，纯标准库）。做法：论文的
8 对基座 + `cand9.py` 生成的全部 50 个“第 9 对”候选 `(r9, q9)`，再枚举 `J0` 的
29 种切分（论文的 `{0,5,6}` 加上所有含新对的 3-子集），逐个用
`paper_construct.build(pairs=..., J0=...)` 构造点集并检查独立性。

```
SUMMARY candidates=50 tested=1450 size_134770=1421 valid=0 conflicts={"row": 1421}
```

- 1,450 个构造里 1,421 个点数恰为 **134,770**（其余 29 个点数不同）；
- **valid=0**：所有点数正确的构造都不独立，且首个冲突全部是**同一行内两个后缀冲突**；
- 这与“自由区 16 点、`α(V') = 8` ⇒ 最多 8 对”的结构论证一致。

结论只覆盖**这一个 8 对基座**：换基座（例如 R1/R2 族）没有扫过。

## `xk_exchange_2026-11-01.log`

`src/scnet/p004/xk.c` 在 C7⁵ 上对 Polak–Schrijver 的 367 点集 `R` 做**完备**的
`k`-交换检查：删掉 `R` 的 `k` 个点、加入 `k+1` 个外部点，看能否得到 368 点独立集。

原理：若 368 点集 `S'` 存在，令 `A = S' \ R`、`Rm = R \ S'`，则 `|A| = |Rm|+1`，
且 `A` 的每个点不能与 `R \ Rm` 相邻，所以 `A` 的 `R`-邻居并集 `T(A)` 满足
`|T(A)| ≤ |Rm|`。于是枚举 `k` 就是完备的。

```
== K=3
NO_3_EXCHANGE nodes=3857 seconds=0.1
== K=4
NO_4_EXCHANGE nodes=57792 seconds=1.6
== K=5
NO_5_EXCHANGE nodes=777655 seconds=31.7
== K=6
NO_6_EXCHANGE nodes=9540882 seconds=440.2
== K=7
NO_7_EXCHANGE nodes=113478118 seconds=5297.8
== K=8
NO_8_EXCHANGE nodes=1272934021 seconds=60426.8
```

结论：**R367 的 8 点对称差邻域内没有 368 点独立集**；任何 368 点集至少要换掉 9 个点。
k=9 按每层约 11 倍增长估计要 10^10 节点、约 9 天，没有跑。
这不是 `α(C7⁵) = 367` 的证明。节点数在两次独立实现（`xk.cpp` 与 `xk.c`）中完全一致。

## `sixm3_conn6_2026-11-02.log`

`--max-k 6 --only-conn6 --threads 1`，输入论文集。先把 k=3..5 的 `connected_pair`
链重建，再跑 `pairStage(6)`：

```
CONNECTED_PAIR_SETS k=3 generated=705413   unique=352504
CONNECTED_PAIR_SETS k=4 generated=3077517  unique=1343540
CONNECTED_PAIR_SETS k=5 generated=17040783 unique=6511209
CONNECTED_PAIR_SETS k=6 generated=111426446 unique=37593864
STAGE connected_pair_k6 sets_or_anchor_instances=37593864 eligible=10207912 max_pool=22 complete=1 found=0
COMPLETE exchanges_up_to=6 no_improvement=1
real 2m26s
```

`conn6` 是“两个 2 点 blocker 相交、再逐层延伸”的族，和 k≤5 的
`connected_pair_k3..k5` 同一构造，补齐了 k=6 阶段表里原本缺的一项；
仍不覆盖“秩恰好为 3”的 6-集。内存峰值约 3.9 GB（3,759 万个 6-集 + 排序缓冲）。

## `sixm3_deep3_k6_all64.log`

`--max-k 6 --only-deep3 --shard i/64 --threads 8`，输入论文集，64 个分片顺序跑。
单片（`shard=0/64`）：

```
STAGE rank3_extensions_k6 sets_or_anchor_instances=27560320 eligible=1257861 max_pool=27 complete=1 found=0
SHARD_STAGE_DONE k=6 shard=0/64
```

64 片合计 **1,750,092,909** 个 6-集、`eligible=80,849,174`，64/64 `complete=1`、
`found=0`、`rc=0`，无 `ERROR`；每片约 117 秒，全部约 2.1 小时（8 线程）。
枚举方式：以每个 3 点 blocker 为锚，逐层与共享一个点的 2/3 点 blocker 求并
（每次至少加 1 点、并集 ≤ 6），按大小 BFS 去重，大小为 6 的直接检查。

## `sixm3_k5_five_sets_2026-11-02.log`

对另外三个已验证的 134,753 集各跑一遍完整 k ≤ 5（`--max-k 5 --threads 8`）：

| 集合 | 用时 | 结果 |
| --- | --- | --- |
| `9pair17` | 394.6 s | `complete=1 found=0` |
| `r1c19` | 380.0 s | `complete=1 found=0` |
| `r2c3` | 386.0 s | `complete=1 found=0` |

33 个 `complete=1` 阶段、0 个 `complete=0`、0 个 `found=1`，三个 `rc=0`。
连同论文集与重排集，五个已知 134,753 集在 k ≤ 5 邻域内均无改进。

## `xk_k9_probe_2026-11-03.log`

`xk_par.c`（并行版，按首点切子树）的正确性与成本核对：

- k=3/4/5/6 节点数 3,856 / 57,791 / 777,654 / 9,540,881，正好是 `xk.c` 的节点数减 1（少根节点）；
- k=6 完整跑：1 线程 422.1 s，8 线程 72.0 s（5.86×），节点数相同；
- k=9：`i0=0` 4,635 个节点（0.5 s），`i0=4000` 单点 5,762,024 个节点（289.8 s），
  `[0,32)` 6,462,565 个节点（97.6 s），`[8000,8032)` 900 s 未结束；
- CP-SAT 对照：k=8 模型 120 s 只推进 2,200 个冲突，`UNKNOWN`，无竞争力。

结论：k=9 全量估计 ≥1.6×10^10 节点，8 线程 2.8 天以上，本机没有跑完。

补充（同一日志）：`xk4.cpp` 是 `xk_par.c` 的统计版，k=6 全量跑出
`NODESTAT nodes=9540881 connected=97026`——只有 1.02% 的搜索节点是连通的。
两版 ESU 式连通枚举都通不过暴力自测，未归档。

## `xk7_connected_2026-11-03.log`

连通枚举（`src/scnet/p004/xk7.cpp`）把 k=9/10 从“跑不完”变成“分钟级”。

- 自测：5,300 个随机实例（m ≤ 18、K ≤ 7），暴力 / 1 线程 / 4 线程三者 0 失配；
  其中 3,161 个实例计数为正，共 30,829 个连通集；
- 真实 R367：k=6 8.4 s、k=7 50.3 s、k=8 258.6 s（均单线程）；
- 容器（15 核 cgroup 配额）：k=8 42.3 s、k=9 208.4 s、k=10 1,010.2 s，全部 `count=0`；
- 本机另一种构建（`xk6`，单线程）k=9 独立复算 1,267.8 s，同样 `count=0`。

结合 k ≤ 8 的完全枚举与 k=0 的直接检查，结论是：R367 不存在“删 k 加 k+1”的
368 点独立集，k = 0..10；任何 368 点集与 R367 的对称差至少 21 点。

⚠️ `xk7` 本身不校验“更小的 k 已排除”，完备性依赖这条定理；单独跑 K=9 没有意义。
