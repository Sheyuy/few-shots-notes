# p009 · Taillard ta41 (30×20) — working notes

Goal: find a schedule with makespan ≤ 2005 (best known UB = 2006, Shylo & Pardalos 2008).
LB = 1915. Machine orders are the certificate format directly.

## Workspace
- `src/instance.h` — instance generated from the verifier's embedded INSTANCE (sha256 checked).
- `src/jssp.c` — tabu search engine (C, single threaded).
- `seed/certificate.json` — the 2006 baseline (verifier: `{"valid":true,"score":"2006"}`).
- `runs/<x>/best.json`, `runs/<x>/log.txt` — long runs.
- `analyze.py` — structural analysis of the baseline solution.

## Verification
C evaluator cross-checked against the official verifier on random schedules:
2220 / 2805 / 5301 — identical in both. The C `evaluate()` is a longest-path
computation on the disjunctive graph, identical in semantics to the verifier.

## Structure of the 2006 solution (analyze.py)
- makespan 2006, critical path has 32 ops, 69 critical ops in total.
- Critical path: m11×5 → m16×5 → m5×3 → m11 → m4 → m15 → m2 → m3×10 → m5.
- Machine 3 is the bottleneck: load 1830, span 1954, only 124 idle.
- Machine 11 load 1675 (288 idle), m16 1625 (376 idle), m5 1709 (266 idle).
- ⇒ the makespan is driven by m3; to gain 1 unit the chain of critical ops on
  m11/m16/m5/m3 has to be re-timed (an operation on the critical path must
  start one unit earlier, or the idle before it must be removed).

## Method
Tabu search on the disjunctive graph.
- Neighbourhoods (bitmask `--nb`):
  - 1 = **N1** (van Laarhoven/Aarts/Lenstra/Ulder'92): swap any adjacent pair of
    *critical* operations on the same machine.
  - 2 = **N5** (Nowicki/Smutnicki'96): swap the first two / last two ops of each
    critical block (⊂ N1).
  - 4 = **N6** (Balas/Vazacopoulos'88): move an op of a critical block to the
    beginning or the end of that block.
  - 8 = **N8** (Xie/Li/Gao/Gui'21): move an *inner* op of a critical block
    outside the block (just before its first / just after its last op).
  - critical block = maximal run of consecutive operations of one machine that
    are all critical (zero slack).
- Tabu: attribute = unordered operation pair (swap) or (machine, op) (insertion);
  tenure drawn uniformly from [Lmin,Lmax] with L = 10 + jobs/machines
  (=11.5 here → [12,16]); aspiration = a move better than the global best.
- Diversification: random feasible adjacent swaps (kick) + restart from the
  best / from an elite pool.
- Evaluation: longest path (Kahn) with an exact early-abort cutoff.

## Dead ends / notes
- A "topological-order single pass" evaluator was implemented (5–10× fewer
  operations) but turned out **slower** than Kahn+early-abort: rebuilding the
  topological order costs 2×600 element copies per neighbour, and the early
  abort already skips most of the work. Kept in the code (`--fast 1`), unused.
- Swapping a *tight* critical arc (u→v with est[u]+dur[u]==est[v]) can only
  *increase* the makespan (v's start is unchanged and u is delayed) — so the
  useful neighbourhood is "any adjacent pair of critical ops", not "tight arcs".
- 5 concurrent search processes on this 2-core box (shared with another agent)
  slow everything down ~5×; the box load was 7.5 during benchmarking, so all
  iteration-rate numbers measured under load are unreliable.

## Log of results
(see below, appended as runs progress)

## Key structural facts (measured)

**The 2006 solution is a very deep local optimum.**
- All 580 adjacent swaps on all 20 machines: **no** improving move (best = 2006, a
  plateau move on machine 16; 2nd best = 2008; worst = 2160).
- All 2001 single-operation insertions of a critical operation to any other
  position on its machine: **no** improving move.
- 53 N1 neighbours (both operands critical) exist; the N1 neighbourhood is steep:
  the best neighbour is on average ~185 units worse than the incumbent.
⇒ any improvement has to come from a *coordinated multi-step* change; plain
  descent can never leave 2006.

**Feasibility is cheap**: a random adjacent swap keeps the disjunctive graph
acyclic with probability 99.8 %, so the search can move freely.

**Machine loads / slack**: max machine load 1830 (machine 3), max job length
1232. The critical path of the incumbent runs m11→m16→m5→m11→m4→m15→m2→m3→m5
(32 operations, 69 critical operations in total).

## Algorithm decisions (measured)

| variant | it/s (CPU) | `cur` makespan after 90 s CPU |
|---|---|---|
| exact evaluation of the whole N1 neighbourhood | 2 655 | 2085 |
| O(1) estimate, top-3 recomputed exactly | 28 400 | 2214 |
| O(1) estimate, top-8 recomputed exactly | 14 800 | 2128 |

⇒ the estimate buys 5–11× more iterations but loses ~30–130 units of search
quality. Since 2006 is a deep optimum where quality matters more than raw
speed, the long runs use **exact evaluation**.

## Implemented
- N1 (van Laarhoven et al. 92) neighbourhood, optional N5/N6/N8 (`--nb` bitmask).
- Tabu: attribute = unordered operation pair, tenure ~ U[12,16] (L = 10 + j/m),
  aspiration = better than the incumbent best.
- Fast exact evaluator: the topological order of the neighbour is obtained from
  the current one by moving one operation past its neighbour; one relaxation
  pass then gives the longest path (25 % of the neighbours need the full Kahn
  fallback).  Verified against the Kahn evaluation on 20 000 random swaps:
  0 mismatches.
- Diversification: feasible random kicks (adaptive strength), restarts from the
  best / from an elite pool (24 solutions), **path relinking** between elite
  solutions (adopt the target's machine orders one machine at a time, reverting
  a machine if that would create a cycle).

## Neighbourhood comparison at equal CPU budget (120 s CPU, seed 909 / 303)

| neighbourhood | it/s (CPU) | `cur` makespan (distance from the incumbent) |
|---|---|---|
| N1 only (`--nb 1`) | ~3 500 | 2048 – 2086 (search drifts 40–80 above 2006) |
| N1 + N6 (`--nb 5`) | ~1 000 | **2006 – 2021** (search stays on the 2006 plateau) |

N6 (Balas & Vazacopoulos 1998: move an operation of a critical block to the
block's start or end) is roughly 3× more expensive per iteration but keeps the
search *on* the 2006 plateau, which is exactly where an improvement to 2005
would have to come from.  The long runs therefore use N1+N6.

Also tried and rejected:
- Coordinated **critical-block moves** (reverse / rotate the whole block): only
  13 critical blocks of length ≥ 2 exist in the incumbent, and the best such
  move is **+101** — far worse than the best single swap (+0).  Not useful.
- **O(1) move estimate** to rank the neighbourhood: 5–11× more iterations but
  30–130 units worse search quality ⇒ not worth it on a deep optimum.
- **Job-reinsertion kick** (implemented, `--kicktype 1/2`) — kept as an option.

## Long runs
| run | binary | neighbourhood | seed | notes |
|---|---|---|---|---|
| C | jssp3 | N1 | 303 | |
| E | jssp6 | N1+N6 | 505 | least-tabu fallback instead of a random move |
| F | jssp7 | N1+N6+N8 | 707 | randomised tie-breaking among equal-makespan moves |

## Critical-path structure of the incumbent (gaps.py)

The critical path of the 2006 schedule is **completely tight**: 32 operations,
zero waiting time between consecutive operations, the first operation starts at
t = 0 and the last one finishes at t = 2006.  The sum of the durations along
the path is exactly 2006.

    m11 ×5  (j24,j18,j12,j28,j14) → m16 ×5 (j26,j9,j15,j5,j18,j14)
    → m5 ×3 (j14,j18,j4,j21) → m11 (j21) → m4 (j21) → m15 (j21,j23)
    → m2 (j23) → m3 ×10 (j23,j27,j5,j14,j19,j25,j26,j24,j16,j10) → m5 (j10)

Consequences:
* no single operation of the chain can be shortened, so an improvement needs a
  *different* chain — a global rearrangement, not a local edit;
* the bottleneck machine 3 (load 1830) contributes 616 units of the chain, and
  the ten chain operations run back-to-back on it (1357 → 1973), i.e. machine 3
  is saturated: only 124 time units of idle exist on it in the whole schedule.
* every single-swap / single-insertion move therefore increases the makespan
  (the chain is tight), which is exactly what the measurements show.

## End-to-end validation of the search machinery

Starting from a *degraded* but feasible schedule (the 2006 incumbent with 40
random adjacent swaps applied, makespan **2744**), the same tabu search reaches
**2006 in 76 iterations (0.7 s)**:

    [init] seed=11 nb=5 makespan=2744
    ... NEW BEST 2261 / 2214 / 2176 / 2137 / 2103 / 2074 / 2049 / 2037 / 2022
    ... NEW BEST 2019 / 2013 / 2006        (iteration 76)

So the engine reliably finds the best-known solution from a poor start; the
difficulty is *only* in improving on 2006.  This also motivates the
`--multistart` option: every restart the incumbent is perturbed with random
feasible swaps, so the descent lands in a different basin and the search
explores many distinct 2006-quality local optima.

## The 2006 plateau is razor-thin (measured on the incumbent)

Critical blocks of the incumbent (machine, start position, length):
`(1,16,14) (3,19,10) (16,4,17) (11,0,6) (5,12,4) (2,1,4) (6,19,4) (15,14,2)`
plus seven blocks of length 1.

* of the **53 N1 moves** (both operands critical) exactly **one** keeps the
  makespan at 2006 (machine 16, positions 18/19); the second best is 2008 and
  the worst 2160;
* of the **60 N6 insertion moves** **none** keeps 2006; the best is 2019.

So 2006 is an isolated spike: every single edit is uphill, and there is
essentially no plateau to walk on.  The cycle detector confirms it — the
back-jump mechanism fires on 76 % of the iterations, i.e. the search keeps
revisiting the very same handful of solutions.

This is why the strategy that works best is **multi-start**: repeatedly
descent into *different* basins (the search reaches 2006 from a random start in
< 100 iterations) and hope that one of the basins bottoms out at 2005.

## Directions not finished (most promising first)

1. **O(1) estimate for the N6/N8 insertion moves** (Balas & Vazacopoulos use a
   head/tail feasibility test: for an insertion of `u` on machine `m` the new
   heads/tails of the machine chain `q … r` and the new machine successors of
   `u` can be written down in closed form).  With that, the whole
   N1+N6+N8 neighbourhood could be ranked in O(1) per move and only the chosen
   move recomputed, i.e. the N6/N8 part would cost ~5–10× less than the
   10 µs Kahn evaluation it uses now.  That is the single biggest lever left:
   N6 is what keeps the search on the 2006 level, and it is currently 3× more
   expensive per iteration than N1.
2. **Shifting bottleneck / single-machine exact re-optimisation** (Adams, Balas
   & Zawack 1988, Carlier's algorithm): re-optimise one machine at a time
   against fixed heads/tails, re-optimising already sequenced machines after
   every insertion.  Gives diverse high-quality starts for 30×20 and is the
   classical partner of tabu search (Pezzella & Merelli 2000).
3. **Path relinking with a local-search burst at every intermediate step**
   (Peng, Lü & Cheng 2015, the recipe that set the records on ta43/ta47/ta50).
   The current implementation only walks the bridge and records the
   intermediate solutions; it never improves them.
4. **Memetic / parallel population** with explicit diversity control
   (Hernandez-Constantino & Segura 2019, who set the ta42 record).

## Run log (all runs)

| run | binary | configuration | iterations | CPU | best |
|---|---|---|---|---|---|
| A | ta41.c | N1, plain tabu | 227 481 | 71 s | 2006 |
| B | jssp.c | N1+N5+N6+N8 | 84 546 | 372 s | 2006 |
| C | jssp3 | N1, fast eval, kick 1000 / restart 5000 | 719 026 | ~190 s | 2006 |
| D | jssp3 | N1, tenure 25–45 | 445 150 | ~120 s | 2006 |
| E | jssp6 | N1+N6, aggressive kick | 300 000 | 286 s | 2006 |
| G | jssp8 | N1+N6, conditional kick (`--kicktrig 3`), restart 20 000 | 300 000+ | 300 s+ | 2006 |
| I | jssp11 | N1+N6 + multi-start (40 random feasible swaps at every restart) | 100 000 | 83 s | 2006 |
| J | jssp13 | N1+N6 + multi-start + back-jump on cycle detection | 100 000+ | 100 s+ | 2006 |
| K | jssp14 | N1+N6 + multi-start, restart 2000 (frequent basin change) | 50 000+ | 40 s+ | 2006 |
| exp2 | jssp.c | exact vs O(1)-estimate evaluation, 90 s CPU each | — | 270 s | 2006 |
| exp3 | jssp4 | N1+N6, 120 s CPU | 122 752 | 120 s | 2006 |

No run ever produced a schedule better than 2006.

---

## 2026-11-01：路径重链接其实**从来没被触发过**（一个真 bug），以及修法

### 1. 诊断：为什么 `--bridge` 跑了 11 轮却毫无作用
`src/jssp.c` 的 `path_relink()` 本身实现得没问题（逐台机器采用目标解的顺序、
遇到成环就回退、每个中间解都 `elite_add` 并检查是否刷新最优 ✅）。

问题在**调用条件**：
```c
if (nbridge > 0 && n_elite > 1) { ... path_relink(elite[tgt], ...) ... }
```
而 `elite_add()` 只在两处被调用 ✅：
* 刷新最优解时（`cur_ms < best_ms`）✅；
* kick 之后（每 `kick_iv` 次停滞）✅。

**ta41 的纪录是 2006，而我们的解已经是 2006** ⇒ 「刷新最优」永远不发生 ✅；
`kick_iv` 默认 3000、我们跑的几轮里 `kicks=0` ⇒ 精英池**永远只有 1 个元素** ✅❌
⇒ `n_elite > 1` 恒假 ⇒ **路径重链接一次都没跑过** ✅✅。
（跑 A–K 十一轮、每轮都有 `--bridge`，全是无效参数 ✅❌。）

### 2. 修法（`src/jssp15.c`）
* **每次 restart 把当前解喂进精英池** ✅：
```c
cur_ms = cur_eval();  calc_tails(topo_, NOPS);
elite_add(mseq, cur_ms);        /* <-- 新增 */
```
这样精英池会装满重启时遇到的**多样化**解（不止是最优解）✅，
`n_elite > 1` 立刻成立 ✅，桥真正开始走 ✅。
* 精英池容量 `NELITE` 24 → **48** ✅（多样性）。

### 3. 新增：**桥上每一步都跑一次局部搜索**（Peng–Lü–Cheng 的配方）
原实现「只记录中间解，从不改进它们」——而那篇论文（正是 ta43/ta47/ta50 纪录的配方）
的做法是**走一步桥、就地局部搜索一次** ✅。

新增 `burst_tabu(iters)` ✅：N1 邻域（同一台机器上相邻两个操作交换）上的小型禁忌搜索
（自带的局部禁忌表 `g_btabu_until[NM][NJ]` ✅，禁忌期 6–10 ✅），
结束时把 `mseq` 恢复成**这段 burst 里最好的那个** ✅。
在 `path_relink()` 里：采用一台机器的顺序 ⇒ 存一份纯桥状态 ⇒ 跑 burst ⇒
若 burst 找到更好的解就更新最优/精英 ⇒ **再把桥状态恢复回来继续走** ✅（保持桥的语义不变 ✅）。

CLI 新增 `--burst N`（每个中间解上跑 N 次迭代，0 = 关闭 ✅）。

### 4. 状态
`src/jssp15.c` 已编译通过并做过冒烟测试（从 best.json 的 2006 起跑 120 s ✅ 无崩溃 ✅，
精英池不再只有 1 个元素 ✅）。真正的长时间跑需要等 CPU —— 目前两个核都被 p003 的
kissat 占着（见 p003 笔记：36 点 (4,6)-图的 SAT 求解）。

---

## 2026-11-01 19:45：p009 也搬到远程算力，以及远程调试踩的四个坑

**远程工作流** `.github/workflows/ta41.yml` ✅：编译 `jssp15.c` → 多进程搜索 →
`pick_best.py` **独立复算 makespan** → 上传 artifact ✅。

### 新工具 `pick_best.py`：独立复算 makespan（不看求解器自己的账本）
在 runner 上直接用 `src/mavis/p009/instance.json` 的 routes+durations，
对每个候选调度在**析取图**上做最长路计算（工序级前驱：同工件序 + 同机器序）✅，
取最优写 `out/best.json`（证书格式 `{"machines": [...]}`）✅。
**交叉验证**：对 best.json（2006）算出 **2006**，与官方验证器 `{"valid":true,"score":"2006"}` 一致 ✅✅。

> 写这个工具时踩了一个**索引 bug**：`durations[j][k]` 是按**路线位置 k** 索引的，
> 我一开始按**机器号 m** 索引 ⇒ 算出 3197 ⇒ 被官方值 2006 打脸 ✅❌ ⇒ 修正后一致 ✅。
> 这类「独立复算」的价值就在这里：它和官方验证器互相印证，任何一方算错都会暴露 ✅。

### 远程调试踩的四个坑
1. **`jssp15.c` 缺 `instance.h`** ⇒ runner 上编译失败（ta41 的实例数据是 `#include` 进来的）✅❌ ⇒ 已推 `src/mavis/p009/instance.{h,json}` ✅。
2. **参数名搞混**：`search3` 用 `--time/--rng/--out`，`jssp15` 用 `-t/-s/-o` ⇒ 一开始传错 ⇒ 修正 ✅。
3. **`--time`（search3）的单位是秒不是分钟** ⇒ LABS 的 4 个远程 job 只跑了 330 秒就"成功"结束，
   产出的 E=678/722/714/730（本地 120 秒 sweep 也只有 770–790 ⇒ 578 是**长时间**跑出来的）✅❌
   ⇒ 改为 `--time $((MIN*60))`，并**加上 `--warm` 从当前最好解（E=578）热启动** ✅（长跑脚本 `long.sh` 一直这么干 ✅）。
4. **artifact 里 `pick best` 步骤找的是 `out/s1.json.log`**（应为 `out/s1.log`）⇒ 步骤失败 ⇒ 改用 `pick_best.py` ✅。

### 时间限制的实测
本地 `-t 60` 实测 `elapsed=62.2s` ⇒ **时间限制是准的** ✅；
远程 5 分钟的 job 实测 5 分 01 秒完成搜索 ✅ ⇒ 之前「超时 2 倍」是我看错了时间戳 ✅❌。

---

## 2026-11-01 20:55：【埋雷】`--nb` 从 enum 静默变成位标志，害我以为在跑 N6

我今天新起一个「N1（迭代率最高，笔记里 run C 的配置）」的实例，
结果 **15 分钟 iters=0、empty=133459** ⇒ 邻域候选**一个都生成不出来** ✅❌。

### 根因
`src/jssp15.c` 的邻域开关是**按位与**判定的 ✅：
```c
if (nbhd & 1) { /* N1 */ }
if (nbhd & 2) { /* N5 */ }
if (nbhd & 4) { /* N6 */ }
if (nbhd & 8) { /* N8 */ }
if (nbhd & 16) { /* 协同关键块移动 */ }
```
但声明处的注释还写着 **枚举**语义：`/* 0=N1 1=N5 2=N6 3=N8 4=N1+N8 */` ✅❌。

⇒ 实际对应关系 ✅：

| `--nb` | 注释声称 | 实际 |
|---|---|---|
| 0 | N1 | **空**（0 与任何位都=0 ⇒ 无邻域 ⇒ 零移动）✅❌ |
| 2 | N6 | **N5**（2&4=0）✅❌ |
| 4 | N6 | **N6** ✅ |
| 9 | — | N1+N8 ✅ |

⇒ **我们历史上所有标着「N6」的跑（`--nb 2`）其实跑的是 N5** ✅❌。
⇒ 已把注释改成位标志语义并写清例子 ✅（`src/mavis/p003/…` 不受影响，这是 p009 独有的 ✅）。

### 修好后
```
--nb 1 → 9216 iter / 12 s、empty=0、cand/iter=30.4 ✅
```
⇒ 本地现在并行跑两个：**`--nb 2`（N5）** 与 **`--nb 1`（N1）** ✅，
远程 3 个 ta41 job 用 `--nb 2`（N5）✅。
**下一轮**应补上 `--nb 4`（N6）与 `--nb 8`（N8）做邻域多样性 ✅（本轮 20 个并发槽已满）。

### 教训
⇒ **参数语义要连着「它到底触发了什么」一起验证** ✅：
「跑通了」不等于「跑的是你以为的那个」✅；
本次是 `empty` 计数把它抓出来的 ✅（`empty=133459` 是个很响的信号 ✅）。

### 【最终台账】**80 进程·小时**（5 种邻域）全部 makespan=2006
远程 8 个 job（N5/N6/N8/N1+N8 + 不同 burst/bridge/seed）⇒ 独立复算全部 2006 ✅❌。
⇒ 「禁忌 + 路径重链接」这条路已充分探索 ✅；
要破 2005 需要更强的邻域（N6/N8 的 O(1) 估计）或 Carlier 移位瓶颈重优化 ❓。

---

## 2026-11-04 14:45：换一条完全不同的算法路线 —— **OR-Tools CP-SAT**

**为什么换** ✅：我们的「禁忌搜索 + 路径重链接」在 ta41 上已经跑了 **80+ 进程·小时**，
五种邻域全部卡在 2006 ✅❌；而 **CP-SAT 有专门为车间调度设计的引擎**
（区间变量 + no-overlap + 工序前序 + portfolio 搜索 ✅），
**和我们用过的任何算法都不一样** ✅。codearts 在 p005 上用 CP-SAT 跑过 **717 万约束** ⇒ 规模上没问题 ✅。

**模型**（标准 job-shop 形式 ✅）：
* 每个工序一个区间变量，size = 加工时间 ✅；
* 同工件：工序 k+1 的开始 ≥ 工序 k 的结束 ✅；
* 同机器：区间两两不重叠 ✅；
* 目标 = min makespan（所有工序结束的最大值）✅。

新工具 **`cpsat_ta41.py`** ✅：读 `instance.json` 的 routes+durations ✅ ⇒ 建模型 ⇒ 求解 ⇒
把解按机器上的开始时间排序，导出成官方验证器要的证书格式 `{"machines": [[job ids …], …]}` ✅，
并用 `pick_best.py` 的**独立复算**交叉验证 ✅（官方验证器再验一遍 ✅）。

**踩到的坑** ✅：`solver.parameters.use_disjunctive_constraint` **不是所有 ortools 版本都有** ⇒
第一轮 4 个 job 全部秒挂在 AttributeError ✅❌；而且我用 `|| true` 把错误吞了 ✅❌
⇒ 已加 `hasattr` 保护 + 不再吞错误 ✅ ⇒ 第二轮 3 个 job 正常在跑 ✅（seed 11/12/13，linearization_level 0/1/2 各一 ✅）。

### 2026-11-04 21:00：CP-SAT 的**第一轮结果 —— 2106，比我们的 2006 差 100**

CP-SAT 的 job **求解本身成功了** ✅，但我的独立复算步骤报 `AssertionError: 2106` ✅
⇒ **它找到的调度 makespan 是 2106** ✅❌（我们的禁忌+路径重链接是 2006 ✅）。

**原因分析** ✅：我把 horizon 设成了「所有工件的总工时」✅ —— 那是个**极松的上界** ✅，
CP-SAT 的区间变量因此几乎没有传播 ⇒ 5.5 小时只做到 2106 ✅。
⇒ **结论**：CP-SAT 的裸模型比我们的禁忌搜索**差一个数量级** ✅（它擅长的是「给定紧上界的可行性」✅）。

### 修正：把 makespan 收紧到 **≤ 2005**
新增 `--cmax-cap` ✅：加上 `cmax ≤ cap` 且把每个开始变量的上界也压到 cap ✅
⇒ 搜索空间从「无上界」缩到「≤2005」这个极小的窗口 ✅。

于是这个模型变成一个**二择一的漂亮问题** ✅：
* 找到 ≤2005 的解 ⇒ **破纪录** ✅；
* 证明 ≤2005 **不可行** ⇒ **2006 是 ta41 的最优值** ✅✅（一个干净的定理 ✅）。

已派 4 个 job：cmax-cap=2005（三个种子/不同 workers ✅）+ cmax-cap=2006（复现纪录 ✅）。

### 2026-11-05 02:20：带 cmax-cap 的 CP-SAT ⇒ **status=UNKNOWN**（既没找到，也没证明）

4 个 job（cmax-cap = 2005/2006，300 分钟）全部返回
`status=UNKNOWN makespan=n/a bound=n/a elapsed=18000s` ✅❌
⇒ **「是否存在 ≤2005 的调度」在 5 小时 / 2 线程内无法判定** ✅（既没找到解，也没能证明不可行 ✅）。

⇒ 这条线的问题不是「模型对不对」而是**搜索力不够** ✅ ⇒ 需要更好的求解策略 ✅。

### 两个工程 bug（顺手修掉）
1. **`--log` 只打到 stdout** ⇒ `out/cpsat.log` 从未生成 ⇒ 工作流「没有文件可打包」⇒
   **产物一直是空的** ✅❌（之前几轮「取不到产物」是这个原因，不是 job 没跑 ✅）⇒ 改为 `| tee out/cpsat.log` ✅。
2. `dispatch_sat.py` 里路径常量 `W` 未定义 ⇒ fetch 静默失败 ✅❌ ⇒ 改为 `WORK` ✅。

### 关键升级：**把已知的 2006 调度作为 hint 喂给 CP-SAT**
CP-SAT 支持 `AddHint` ⇒ 从 2006 的机器顺序算出每个工序的最早开始时间 ⇒ 全部作为提示 ✅。
⇒ CP-SAT **从一个已知可行的解出发**，它的 LNS（large neighborhood search）就能在这个解上做局部改进 ✅✅
⇒ 这比「从零搜索」强得多 ✅（这正是 CP-SAT 在调度题上的标准用法 ✅）。
同时打开 `interleave_search` ✅。
已派 4 个（cap = 2005 ×3 + 2006 ×1）✅。

### 2026-11-05 04:10：hint 的计算逻辑**本地验证通过**（上一轮是静默失败）
in-progress job 的日志还拿不到（GitHub 完工才生成 blob ⇒ 404），所以我把**那段调度计算**单独抽出来本地验 ✓：

* 用修正后的拓扑序（工件前驱 + 机器前驱都就绪才排）算 600 个工序的开始/结束时间 ✅；
* 得到的 makespan = **2006** ✅，与 `pick_best.py` 的**独立复算**完全一致 ✅✅。

⇒ 这次喂给 CP-SAT 的 hint 是**一条真正可行的 2006 调度** ✅ ⇒ LNS 可以在它上面做局部改进 ✅
⇒ 上一轮那批（KeyError 静默失败）等于没带 hint，结论不能作数 ✅❌（已重派 ✅）。

⇒ 教训（第三次了 ✅）：**静默的 except 会让「看起来在跑」的东西其实没生效** ✅。
⇒ 以后所有 `except: print(...)` 都要改成**大声失败**（`!! HINT FAILED` + 继续但标注）✅。

### 2026-11-05 07:25：旧批次（无 hint）的最终判决 = UNKNOWN，结论作数
18:23 UTC 派发的 4 个 job（**用的还是破 hint 的旧代码** ⇒ 日志里明确写着 `hint failed: (23, 2)` ✅）
⇒ 5 小时后 `status=UNKNOWN makespan=n/a bound=n/a elapsed=17997s` ✅
⇒ **这批结论作数**（等于又一次「无 hint 的 cap 版」✅，与 02:20 那批一致 ✅）。

⇒ **真正带 hint 的第一批（19:18 UTC = 03:18 CST 派发）于 00:18 UTC = 08:18 CST 结束** ✅
⇒ 那是 hint 路线的**第一次有效检验** ✅（前两批都没带 hint ⇒ 都不能算数 ✅）。

⇒ 教训再记一次：**「job 跑完了」≠「job 跑的是我以为的那个东西」** ✅
⇒ 以后每个 job 的日志里必须先确认关键开关生效（hint 行 ✅ / 命令行回显 ✅），再看结论 ✅。

### 2026-11-05 08:25：**hint 路线第一次有效检验** ⇒ hint 生效，但 CP-SAT 仍判不了 2005

4 个 job（19:18 UTC 派发，**真正带 hint 的第一批** ✅）跑完 5 小时，日志**逐条确认 hint 生效** ✅：

| seed | cap | 判决 | 含义 |
|---|---|---|---|
| 71 | 2006 | **FEASIBLE makespan=2006.0 bound=1892.0** | incumbent 可行 ✅（官方验证器 `{"valid":true,"score":"2006"}` ✅，canonical `73b01ead…` ✅ 与我们本地解同一个同构类 ✅） |
| 72 | 2005 | UNKNOWN | 5 小时判不了「≤2005 有解吗」 ❌ |
| 73 | 2005 | UNKNOWN | 同上 ❌ |
| 74 | 2005 | UNKNOWN | 同上 ❌ |

⇒ **hint 机制本身完全正确** ✅（`hint: incumbent makespan 2006 over 600 ops` ✅，与 `pick_best` 的独立复算一致 ✅）
⇒ 但 **CP-SAT 的下界只做到 1892**，离 2005 还差 113 ⇒ **搜索力不够** ✅
⇒ 且注意：**ta41 的最优值至今未知** ✅（社区没人知道 2005 是否可行 ✅）
⇒ 所以这题判定「≤2005 可行性」本身**就是研究级问题** ✅，5 小时 × 2 线程不足 ✅。

⇒ **p009 的结论**：2006 站得住 ✅，破纪录需要远超我们预算的算力 ✅。

### 2026-11-05 18:35：p009 最后一轮（带 hint）⇒ **UNKNOWN ×2 + FEASIBLE 2006（bound 1891）**
| seed | cap | 判决 |
|---|---|---|
| 81 | 2005 | UNKNOWN |
| 82 | 2005 | UNKNOWN |
| 83 | 2006 | FEASIBLE makespan=2006.0 bound=1891.0 |

⇒ **hint 每次都生效** ✅，但 **5 小时 × 2 线程的下界只到 1891**，离 2005 差 114 ⇒ 判不了 ✅。
⇒ **p009 结算：2006 站得住** ✅（社区至今也不知道 ta41 的最优是不是 2006 ✅）。
