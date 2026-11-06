# p014 源码

最短 29 阶 Golomb 尺，基线 623（Singer 构造），目标 ≤ 622。本轮只做可行性探针，
没有找到 622，也没有证明 623 最优。

## 文件

| 文件 | 作用 | 结论 |
| --- | --- | --- |
| `golomb.c` | 定长 `L` 上的模拟退火，代价 = 重复差分对数（0 即合法）。含 `--selftest`：随机状态上逐差分的 delta 更新与全量重算对拍（输出 `DELTA_OK`） | L=623 基线代价 0；L=622 热启动代价 14，T=0.2..3.0 共 2×10⁷ 步**从未低于 14** |
| `golomb2.c` | ±1 邻域禁忌搜索 + 平台容忍（允许等价代价移动） | L=622 上 16,729,446 次迭代，代价**冻结在 14** |
| `cpsat_probe.py` | CP-SAT：29 个位置变量、`p0=0`、`p28=L`、all-different、两两差值不等；支持给 hint 与多 worker | 不给 hint 时 60 s 连**已知存在的 623** 都找不到（`UNKNOWN`）；给 hint 0.1 s |
| `cpsat.py` / `cpsat2.py` | 更早的两版 CP-SAT 探针（被 `cpsat_probe.py` 取代） | 同上，仅留档 |

## 复现

```bash
gcc -O3 -march=native golomb.c -o golomb
# 用法: ./golomb <L> <秒> [种子] [随机起点0/1] [初温T]
./golomb 623 60 88172645463325252 0 3.0   # 基线种子 → 代价 0（自检通过）
./golomb 622 60 88172645463325252 1 3.0   # 随机起点 L=622 → 收敛到 30 上下

gcc -O3 -march=native golomb2.c -o golomb2 && ./golomb2   # ±1 禁忌，L=622，代价冻结 14

python3 cpsat_probe.py 622 60               # 无 hint：UNKNOWN
python3 cpsat_probe.py 623 60 --hint 1      # 有 hint：0.1 s 求解
```

`golomb.c` 的 `--selftest` 是另一个入口（随机状态上对拍 delta 更新与全量重算，
输出 `DELTA_OK`），与上面的定长搜索分开调用。

三种方法（退火 / CP-SAT / ±1 禁忌）互相独立，都停在同一个 14 上；题面复审也已
扫尽 Singer/Bose 构造的所有单位与窗口。结论是"这一族构造给不出 622"，
**不是**"622 不存在"。
