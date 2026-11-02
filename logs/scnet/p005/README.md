# p005 运行日志（2026-10-31）

这些是原始 stdout，保留 `UNKNOWN`、超时和被中断的运行，不做事后加工。
每份日志只说明“这次运行在给定参数下既没找到 513，也没证明 513 不可行”，
两个方向都不能反推。

| 文件 | 关键行 | 读法 |
| --- | --- | --- |
| `cp_solve2.log` | `clauses 7173360 build 37.2s` … `status UNKNOWN t=967.1` | 7,173,360 = F₃⁸ 的线数，即全局 `sum(x) ≥ 513` 模型；967 秒未判定 |
| `cp_alt.log` | `status UNKNOWN t=344.5` | 变体运行，只留状态行，参数未记录 |
| `cp_hint.log` | `status UNKNOWN t=343.8` | 带 512 hint 的变体，同上 |
| `two_slice.log` | `cap clauses 1593594 cross 419904 build 10.8s` … `UNKNOWN t=911.4` | 固定 x₀=0 整层（192 点）的子问题 |
| `two_slice_f1.log` | `UNKNOWN t=3607.0` | f=1 变体，1 小时 |
| `two_slice_hint.log` | `UNKNOWN t=610.6` | 带 hint 的层子问题 |
| `two_slice_long.log` | `cap clauses … build 9.2s` / `solve` | 长跑被中断，没有状态行 → 未判定 |
| `product.log` | `d 6 K 112 UNKNOWN` / `A size None t=120` | F₃⁶ 子搜索 120 秒只到 85 点，未找到 112 |
| `rk13.log` | `k10 points 384` … `FINAL max addable after 10-removal: 2 (x=2472)` | 结构化删点 KK=10：删 10 点最多加回 2 点 |

## 一份不作为证据的日志

`cp_solve.log` 也在目录里，但**不要引用**：它只有
`built 7171200 clauses in 57.0s` / `status OPTIMAL t=340.4` / `size 1154`。
生成它的脚本版本已无法确认，`size` 统计的是什么也没法从日志判断；如果 1154 真是
帽集大小，那会是远超 512 纪录的结果，必须独立复核——本轮没有做。所以它既不能支持
“找到更大的帽集”，也不能支持“513 不可行”。保留它只是为了让日志目录完整。
（顺带一个概念点：纯满足模型的 `OPTIMAL` 只表示“找到并证明了可行解”，不是“最优值”。）

## 两个容易误读的地方

1. `rk13.log` 的文件名来自 `rk13.c`，但这条记录实际是 `#define KK 10` 的 `r10.c`
   跑出来的（打印字符串仍是 “10-removal”）。**KK = 11/12/13 没有日志**，
   不能把这条当成“r ≤ 13 穷举”的证据。
2. `two_slice_long.log` 只有构建行和 `solve`，没有 `status`。它证明的是“跑过”，
   不是“跑完”。

## 没有原始日志的记录

下面这些只出现在报告正文，标注为“远端记录”，复现需要重跑：

- 640 核、600 个随机种子的最小冲突搜索：最好能量 E=7，没有 E=0；
- k=6 的部分覆盖计数（`natural_k6` 3,003,919 个 6-集、`rank5_extensions`
  41,986,400 个实例）；
- 240 GiB cgroup 下 3 个完整模型 OOM。

远端机器（DevEnv_428476）在本轮结束时 SSH 授权已失效，没有重新申请，也没有重放旧令牌。
