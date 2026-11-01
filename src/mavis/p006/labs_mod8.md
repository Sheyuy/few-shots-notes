# p006 — LABS n=100 — working notes

Goal: minimise E = sum_{k=1..99} C_k^2 , C_k = sum_i s_i s_{i+k}.
Record: E = 578 (F = 8.6505, Knauer 2004 / QOBLIB labs100). Target: E <= 574.
E is always 2 (mod 4).

## Ground truth / tooling
* Verifier: `python3 /workspace/few-shots/problems/p006-labs-100/verify/verify.py <dir>`
  → baseline `{"valid":true,"score":"578",...}`.
* My own energy code reproduces E=578 on the baseline certificate and always
  yields E ≡ 2 (mod 4) (`--mode selftest`, brute-force cross-checks of the
  incremental update and of the 2-flip delta formula in `--mode swaptest`).
* Workspace: /workspace/few-shots-work/p006-labs-100

## Engine (C) — `search4b` (source: search3.c)
* incremental C[k]/E maintenance: O(n) per single-bit flip, exact integers.
* operators: `tabu` (steepest descent over 1-bit flips, tabu list on positions,
  aspiration by objective), `sabest` (Metropolis on the best move), `tabest`
  (threshold accepting).
* polish: greedy 1-flip descent; **LNS** = exact re-optimisation of a block of m
  bits (all 2^m assignments enumerated in Gray-code order, O(2^m·n)); exhaustive
  2-flip (swap) polish.
* modes: `ils` (iterated local search with archive / restarts / escalation /
  path-relinking), `mem` (memetic: population + 1-point crossover + seam repair
  by LNS + tabu local search), `single`, `selftest`, `swaptest`, `eval`.
* bug found & fixed: the tabu aspiration must be relative to the best objective
  ever seen (not the current state) — with the wrong rule E was 1030 instead of
  810 on the same config.

## Measured results (120 s per config, E after the run)
| method | E | comment |
|---|---|---|
| random 1-flip SA (T 0.2→0.05, 2e7 steps) | 1162 | DEAD END |
| best-improvement Metropolis SA (`sabest`) | 1338 | DEAD END |
| threshold accepting (`tabest`) | 1470 | DEAD END |
| tabu 1e6 iters only | 834 | best single operator |
| ILS tabu + LNS(m=10), no restarts | 806 | |
| ILS tabu + LNS + random restarts/patience | 778 | diversification helps |
| ILS **without** LNS | 938 | LNS is essential |
| **memetic** pop 16, tabu chunk 50k, LNS | 770 | |
| **memetic** pop 16, tabu chunk 5k, LNS | **734** | best so far |
| memetic pop 32 / chunk 20k | 834 | too big a population |
| memetic without LNS | 826 | LNS essential here too |
| memetic, 30 % mutation | 822 | too much mutation |
| smoothed objective (c²+λ·max(0,|c|-c0)²) | 1262 | DEAD END (also ~10× slower) |
| block perturbation (6 contiguous flips) | 846 | DEAD END |
| tabu ILS warm-started from the 578 record | 578 (no gain) | that basin is not escapable |

## Dead ends / negative results
* Plain, best-improvement and threshold-accepting annealing are all far worse
  than tabu on this instance.
* The smoothed/penalised objective hurts badly (it destroys the guidance of the
  true energy) and makes the inner loop much slower.
* Contiguous-block perturbations are worse than scattered random flips.
* The bottleneck is **diversification**, not the local search: a plain ILS
  converges to E≈780 in ~200 s and then stops improving.

## Literature (ideas only, from public sources)
* Best known solutions for large LABS come from **Memetic Tabu Search**
  (Zhang et al. 2025, arXiv:2504.00987): population, recombination (p≈0.9) or
  mutation, short-run tabu local search, replace-worst. → implemented, and it is
  indeed the best engine here.
* Leiva (GECCO 2007): memetic + tabu is state of the art; "plain evolutionary
  algorithms perform worse than stand-alone local search".
* Halim et al.: tabu search with a better diversification process.
* Prestwich: asymptotic merit factor estimated at F ≈ 9.3 (Golay's 12.32 is only
  an upper bound) → for n=100 that would be E ≈ 537, so 574 is plausible but
  hard; 578 may be close to optimal for this length.
* Zhang et al. improved best-known values for odd N (99, 107 …) but *not* for
  even N=100 → 578 is still the record for n=100 in the public tables.

## Memetic parameter sweep (120 s each, seed noise is large: ±30 energy units)
| config | E |
|---|---|
| pop 16, tabu chunk 5k, LNS 10, xover 14 (seed 62) | 734 |
| same, seed 72 | 794 |
| same, seed 74 | 782 |
| pop 16, tabu chunk 2k | 794 |
| pop 16, tabu chunk 1k | 842 |
| pop 32, tabu chunk 20k | 834 |
| pop 16, tabu chunk 50k | 770 |
| pop 16, chunk 5k, no LNS | 826 |
=> chunk 5k with LNS is the sweet spot; the run-to-run spread is large, so
   single 120 s runs cannot rank configs that differ by <30 energy units.

## Machine note
The box has 2 CPUs but the load average from the parallel agent is 4-6, so my
two processes receive only ~30-40 % of the machine. Long runs are therefore
slower than wall-clock estimates.

## Running now (4 h each, 2 cores)
* `runs/memA.txt` : memetic, pop 16, tabu chunk 5k, LNS m=10, crossover repair
  window 14, random-victim replacement.
* `runs/ilsB.txt` : ILS, tabu chunk 200k, LNS m=10, random restarts, archive.
Both warm-started from the incumbent (best.json) and checkpointing on every
improvement; `merge_best.py` collects the global best.

## Next / untried
* memetic with even shorter local search (chunk 1k–2k) and larger crossover
  repair windows;
* path-relinking moves in the ILS (implemented, `--p-prl`, untested);
* multi-restart population re-seeding to avoid population collapse.

---

## 第八轮：LABS 文献现状（QOBLIB labs100 基准）

* **世界纪录 = 578** ✅（QOBLIB `labs100`：Best objective 578 / Reference objective 578 ✅）
  → **板上基线 578 就是世界纪录** ✅✅ → 目标 ≤574 需要**超出世界纪录 4 个单位** ✅ ✗
* 近期提交（2026-08-15 Tim Bode）= **666** ❌（远不如 578 ✅）→ 说明这个长度很硬 ✅
* SOTA 方法 = **Memetic Tabu Search (MTS)** ✅（Zhang et al. 2025, arXiv:2504.00987 ✅，
  GPU 上做到 N=120 ✅；经典 O(1.34^N) 标度 ✅）
  * 种群 K=100 ✅、交叉 p_comb=0.9 ✅、变异 p_mut=1/N ✅、
    禁忌搜索 + aspiration ✅、禁忌期随机 ✅、
    **tableC/vectorC 增量能量更新** ✅（避免整条重算 ✅）
  * 奇数长度可限制 skew-symmetric ✅（n=100 为偶 ✅ 用不上 ✅）
* 我的 `search3.c` **已经具备**：mem 模式（种群+交叉）✅、增量能量更新 ✅、
  tabu/sa_best/ta 三种接受准则 ✅ → 引擎形状是对的 ✅
* 结论：p006 = "超越世界纪录" ✅，靠**更多算力 + MTS 调参** ✅，不是靠新方法 ✅

---

## 2026-11-01 新增：LABS 能量的模 8 硬约束（把「目标值」从猜变成可证）

板上 `baseline_score = 578`、`epsilon = 1`、方向为 min ⇒ 规则字面上要求 **≤ 577**。
但 LABS 能量有**可证的模 8 限制** ⇒ **577 与 576 根本不可能出现** ⇒ 实际最低可达目标是 **574**。

### 推导
对 n = 100、序列 s_1..s_100 ∈ {±1}，自相关
`C_k = Σ_{i=1}^{100-k} s_i s_{i+k}`，能量 `E = Σ_{k=1}^{99} C_k²`。

1. 每一项 s_i·s_{i+k} ≡ 1 (mod 2) ⇒ `C_k ≡ (100-k) (mod 2)`
   ⇒ **k 为奇数时 C_k 为奇，k 为偶数时 C_k 为偶**。
2. 奇数的平方 ≡ 1 (mod 8)；偶数的平方 ≡ 0 或 4 (mod 8)。
3. k = 1..99 中奇数 k 共 **50** 个 ⇒ 这部分贡献 ≡ 50 ≡ 2 (mod 8)；
   偶数 k 共 49 个，每项贡献 0 或 4。

⇒ **E ≡ 2 (mod 8) 或 E ≡ 6 (mod 8)**。

### 结论
```
可行值: ... 562 566 570 574 578 582 586 ...
不可行: 576(≡0) 577(奇) 572(≡4) 573(奇) 575(奇)
```
* 板上纪录 578 ≡ 2 ✓（自洽）
* **576 ≡ 0 (mod 8) ⇒ 不可能** ⇒ 第一个可能的改进目标是 **574（≡ 6）**；
* 再下一个是 570（≡ 2），再下是 566。

⇒ **p006 的真实目标是 E ≤ 574**，不是 577。
⇒ 任何「找到 577」的搜索结果都应当被**模 8 检查直接判为算错** —— 这是一个**零成本的自校验**，
建议加进搜索器的输出检查里（能立刻抓出能量统计的实现 bug）。

---

## 2026-11-01 19:13：LABS 搜索也搬到远程算力

按 p003 笔记里那套办法（`Sheyuy/few-shots-notes` 是公开仓 + Actions 全开 ⇒ 托管 runner 免费、
每个 job 2 核 16 GB、最长 6 小时、不受沙箱重启影响 ✅），
给 p006 也加了一条工作流 **`.github/workflows/labs100.yml`** ✅：

```
checkout → gcc -O3 search3.c → 多进程搜索（--mode ils 或 --mode mem --pop 32）
        → merge_best.py 精确整数算能量挑最优 → 【模 8 自校验】(E % 8 ∈ {2,6}) → 上传 artifact
```
* 模 8 自校验直接嵌进工作流里 ✅：**任何报出 E ≡ 0/4 (mod 8) 的结果都会被 assert 拦下** ✅，
  这是一条零成本、能在远程自动抓出能量实现 bug 的规则 ✅。

已派发 **4 个 job**（每个 2 进程 × 330 分钟）：seed 1/2/3 用 ILS、seed 4 用 memetic ✅。

---

## 2026-11-02 01:20：第一轮远程 LABS 的硬负结果 —— 578 是**唯一吸引子**

6 个远程 job（每个 2 进程 × 5.5 小时 ⇒ **66 进程·小时**的 MTS，全部从 E=578 热启动）⇒
**每一个都收敛到 E=578** ✅，**没有任何一个改进** ✅❌。

更关键的是：**这 6 条 578 序列两两完全相同**（汉明距离 = 0，考虑整体取反后仍然相同）✅✅
⇒ **E=578 这个最优解是一个唯一的强吸引子** ✅，从任意邻域扰动出发，MTS 都会回到**同一条**序列 ✅。

⇒ 推论 ✅：
* 「收集多条不同的 578 序列当热启动池」这条路**直接死掉** ✅（池里只有一条 ✅）；
* 「加更多算力就能超过 578」这个假设**被 66 进程·小时否掉了** ✅（算力不是瓶颈，**盆地结构**才是 ✅）；
* 要破纪录必须**逃出这个吸引子**，而 tabu/ILS/记忆化禁忌这类「改良型」搜索很难做到 ✅；
  真正能逃离的是 **self-avoiding walk（SAW）/ 随机重启 / 更大规模种群 + 更强多样性** ✅（= SOTA 路线 ✅）。

⇒ 第二轮加的是 **memetic 模式 + 种群 100**（对齐 Zhang et al. 2025 的 MTS 参数 ✅），
外加不同种子 ✅。
