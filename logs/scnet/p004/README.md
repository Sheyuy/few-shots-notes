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

## `sixm3_rank4_k6_shards_0-15of64.log`

2026-11-01，`--max-k 6 --only-deep6 --shard i/64 --threads 1 --seconds 300`，
`i = 0…15`，串行依次运行。每一行 `STAGE rank4_extensions_k6 … complete=1 … shard=i/64`
是一个分片的完整结果。

- 16 个分片全部 `complete=1`、`found=0`；
- 合计 **161,031,153** 个实例（每个分片约 1,000 万）；
- 覆盖 `rank4_extensions_k6` 的 **16/64 = 25%** 锚点。

### 分片的含义

`--shard i/n` 只作用于按 head 索引的循环（`naturalStage`、`anchorStage`），
把下标限制为 `i mod n`。`connectedPairs` 的生成不参与分片，所以**不要**把分片的
`connected_pair_*` 行相加（每个分片都会重跑全量）。这份日志只用了 `--only-deep6`，
因此没有 k≤5 的重复行。

### 还没覆盖的部分

- `rank4_extensions_k6` 的 48/64 个分片（单进程约 48 分钟）；
- `rank3_extensions_k6` 整段（只会更慢）；
- 连通 6-集（`pairStage(6)`，源码里没有实现）。

因此这份日志**不能**支持“k=6 已排除”，只能支持“rank-4 锚点扩展的前 25% 没有改进”。
