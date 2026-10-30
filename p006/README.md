# p006 · Low Autocorrelation Binary Sequence of Length 100

**纪录**: E=578
**方向**: min
**本轮结果**: 742（劣于基线，未突破）

## 基线复现

```bash
python3 problems/p006-labs-100/verify/verify.py problems/p006-labs-100/baseline
# 输出 valid:true, score:578
```

E = Σ_{k=1}^{99} (Σ_{i=1}^{100-k} s_i s_{i+k})²，s_i ∈ {+1, -1}。

## 实验 1：禁忌搜索（`search.c`）

- tenure=8：最好 E=862。
- tenure=14：最好 E=742。

## 实验 2：模拟退火（`sa.c`）

- 每 2 秒一个退火周期，T 从 300 降到 0.02，从最优解重启并扰动。
- 30 秒跑完：最好 E=806。

## 结论

朴素单点翻转邻域（每次 O(N)）离 574 很远。LABS 是已知 NP-hard 问题，N=100 的最优解需要更专门的算法（如 MeritFactor 优化、L-BFGS、或基于谱的构造）。

## 未尝试的方向

- MeritFactor 优化（专门针对 LABS 的连续松弛 + 梯度下降）。
- 基于自相关序列的构造法（如 Skewsymmetric 序列）。
- 用 SAT 编码小规模局部搜索（把当前最优解附近的小窗口编码为 SAT，精确搜索）。
