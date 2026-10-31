# p005 源码（2026-10-31 轮次）

F₃⁸ 中的帽集：目标 513（纪录 512）。这一轮没有找到 513，也没有证明 513 不存在。
每个数字都标了强度：**精确**（穷举/证明）、**有限范围**、**未判定**（`UNKNOWN`/
`FEASIBLE`、超时、OOM 或启发式）。

## Python

| 文件 | 作用 | 强度 |
| --- | --- | --- |
| `make_baseline_idx.py` | 把基线证书转成整数索引 `baseline.idx`（C 程序的输入） | — |
| `analyze.py` | 集外点的冲突数直方图；本轮复算：最小 4，≤2 的点 0 个 | 精确统计 |
| `transversals.py` | 冲突数 = 4 的 16 个点、它们的横截是否互不相交 | 精确统计 |
| `symm.py` | 基线在平移/仿射变换下的自交与秩 | 精确统计 |
| `repair.py` | 破坏-重建：删少量点后贪心补到 513 | 启发式 |
| `slices.py` | 按坐标层切分基线 | 精确统计 |
| `probe.py` | 单点/单层的可行性探测 | 有限范围 |
| `lincheck.py` | 线性（仿射）独立性检查 | 精确 |
| `two_slice.py` | 固定 x₀=0 整层（192 点），另两层求 ≥321 | 有限范围，`UNKNOWN` |
| `two_slice_f.py` / `two_slice_hint.py` / `two_slice_long.py` / `two_slice_rand.py` / `two_slice_remote.py` | 同上的 f=1 变体、hint 变体、长跑、随机层、远端版 | 有限范围，全部 `UNKNOWN` |
| `solve513.py` / `solve513b.py` / `solve_hint.py` / `solve512alt.py` / `cp_full_remote.py` | 全局 6,561 变量、`sum(x) ≥ 513` 的 CP-SAT 及其变体 | 全部 `UNKNOWN` |
| `cap6.py` | 在 F₃⁶ 里搜 112 点帽集（乘积路线的子问题） | 只到 85，未完成 |
| `product_try.py` | 乘积构造尝试 | 子问题 `UNKNOWN` |

## C（历史版本，本轮未重跑，除注明外）

| 文件 | 作用 |
| --- | --- |
| `r10.c` / `rk.c` / `rk12.c` / `rk13.c` | 结构化删点：对每个集外点取 KK=10/11/12/13 个互不相交冲突对，枚举“各删一端”，统计可加点数。**只保留 KK=10 的日志**；打印字符串仍写 “10-removal” 是历史遗留 |
| `capsearch.c` / `capsearch2.c` | 带扰动/禁忌的帽集搜索 |
| `deep.c` | 深度扰动 + 重启 |
| `oneswap.c` | 单点交换扫描 |
| `grow.c` / `grow2.c` | 增长式构造（两个版本） |
| `anneal.c` | 模拟退火，能量 = 冲突线数 |
| `basin.c` | 盆地跳跃（不同半径重启） |
| `greedy.c` | 贪心构造，多随机序取最好 |
| `checkcore.c` | 第三点表与冲突计数的自检 |
| `cap2rm.c` | 删一对点后统计可加点数 |

C 程序都从工作目录读 `baseline.idx`：

```bash
mkdir -p /tmp/p005run && cd /tmp/p005run
python3 /path/to/src/scnet/p005/make_baseline_idx.py > baseline.idx
gcc -O3 -march=native /path/to/src/scnet/p005/r10.c -o r10 && ./r10
# k10 points 384
# best 2 x=2472 mask=8160
# FINAL max addable after 10-removal: 2 (x=2472)
```

## 本轮结论（和它们的强度）

| 结论 | 强度 |
| --- | --- |
| 基线 512 点，`canonical_hash=f76ee04c...`；**0 个可加点** | 精确（本轮复算） |
| 三层切片大小为 192 / 160 / 160 | 精确（本轮复算） |
| 集外每点冲突数 ≥ 4；冲突数 = 4 的 16 个点，其 4 对互不相交 | 精确（`analyze.py` + `transversals.py`） |
| 这 16 个点各自的 256 种横截里，没有一个能同时解锁 ≥2 点 | 精确 |
| ⇒ 插入任何集外点至少要删 4 个基线点（净 −3 起） | 精确 |
| 结构化删点 KK=10：删 10 点后最多加回 2 点（净 −8） | 精确，限该族；日志保留 |
| KK=11/12/13 版本 | 代码在，**未保留运行日志** → 未判定 |
| 增长式构造最好 328 | 启发式 |
| 640 核 600 种子最小冲突：最好 E=7，无 E=0 | 启发式（远端记录，日志未归档） |
| 全局 K=513 CP-SAT：本地/远端均 `UNKNOWN` | 未判定 |
| 固定单层与随机 ℓ 层子问题：`UNKNOWN` | 未判定 |
| 3 个完整模型在 240 GiB cgroup 下 OOM | 未判定（资源不足） |
| F₃⁶ 子搜索只到 85 点（`cap6.py`，`data/scnet/p005/cap6_best.json`） | 搜索失败，不是不存在 |

## 为什么不能写成“513 不存在”

- 所有 CP-SAT 结果都是 `UNKNOWN` 或 `FEASIBLE 512`，没有一次 `INFEASIBLE`/`UNSAT`。
- “插入代价 ≥4”只说明基线附近没有便宜的改进；513 完全可能来自无关构造。
- α(F₃⁶)=112 是已知可达的，本地只到 85 说明子搜索太弱，不能据此排除乘积路线。

因此 p005 的状态是：**512 仍是最好的可验证值，513 需要新数学构造。**
运行日志见 [`logs/scnet/p005/`](../../logs/scnet/p005/)。
