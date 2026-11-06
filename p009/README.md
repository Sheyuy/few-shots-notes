# p009 · TA41 作业车间调度（30×20）

**基线**: makespan **2005**（官方 verifier 的 baseline）
**本轮最好**: **2006**（未突破，未提交 shot）

## 做了什么

四种元启发式 + 单文件版本，全部从同一实例出发：

| 实现 | 文件 |
| --- | --- |
| 禁忌搜索 | `p009/jobshop_tabu.c` |
| 迭代贪心（IG） | `p009/jobshop_ig.c` |
| 模拟退火（SA） | `p009/jobshop_sa.c` |
| 混合遗传算法（HGA） | `p009/jobshop_hga.c` |
| 单文件版（SCNet 轮次） | `src/scnet/p009/jssp.c` |
| 实例诊断 / 证书转换 | `p009/diag.c`、`p009/to_cert.py` |

## 证据

`logs/p009/` 下 22 份以 2006 收尾的运行日志合计 **1,002,838,046** 次迭代，`best` 恒为 **2006**；
其中 HGA 的两份日志更差（2472 / 2567）。`results/p009/` 保存了 33 条历史调度。

官方基线 2005 由 `p009/` 的 orders 直接验证；我们从未产出 2005 或更好的调度。

## 复现

见 `src/scnet/p009/README.md`（单文件版）与 `REPRODUCE.md` 第 8 节。
