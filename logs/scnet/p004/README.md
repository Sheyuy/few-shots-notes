# p004 运行日志

## `sixm3_k6_600s.log`

2026-11-01，本机串行（macOS 无 libomp，用 `/tmp/ompstub` 的 OpenMP 桩编译，
实际 1 线程），`--max-k 6 --threads 8 --seconds 600`，输入是 arXiv/论文构造的
134,753 点集（`data/scnet/p004/cc_10_7_134753_paper.txt`）。

```bash
g++ -x c++ -std=c++17 -O3 -I/tmp/ompstub sixm3.c /tmp/ompstub/omp.c -o sixm3
./sixm3 --self-test
./sixm3 --input ../../data/scnet/p004/cc_10_7_134753_paper.txt --max-k 6 --threads 8 --seconds 600
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
