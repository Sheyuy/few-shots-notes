# p009 源码

TA41（30×20 job shop，基线 makespan 2005）。本目录放 SCNet 轮次的单文件版本，
早期四种元启发式在仓库根的 `p009/`。

## 本目录：`jssp.c`

单文件、无第三方依赖。从当前目录读两个输入：

- `instance.txt`：首行 `30 20`，随后 30 行 × 20 组 `机器 时长`；
- `orders.txt`：20 行 × 30 个工序号，初始可行调度（没有它就 `fopen` 失败，程序会崩）。

先算 `baseline makespan`，随后按关键路径做局部搜索，每次改进打印
`iter <n>: new best <mk> (t=<秒>s)`，达到 2005 时打印 `TARGET REACHED` 退出，
并把当前最优写到 `best_orders.txt`。

```bash
gcc -O3 -march=native jssp.c -o jssp
./jssp
# baseline makespan 2006      （2026-11-06 用 p009/orders.txt 复跑得到的真实输出）
```

注意：`printf` 没有 `fflush`，输出重定向到文件时是块缓冲，看起来像"没输出"——
用 `script -q /dev/null ./jssp` 或终端直接跑即可看到。

`best_orders.txt` 与 `p009/to_cert.py` 兼容：

```bash
python3 p009/to_cert.py best_orders.txt > certificate.json
```

## 早期版本（仓库根 `p009/`）

| 文件 | 说明 |
| --- | --- |
| `jobshop_sa.c` | 模拟退火（交换/插入邻域） |
| `jobshop_tabu.c` | 禁忌搜索 |
| `jobshop_ig.c` | 迭代贪心 |
| `jobshop_hga.c` | 混合遗传算法（种群 + 局部改进） |
| `diag.c` | 实例诊断：机器负载、工序数、下界 |
| `to_cert.py` | 20×30 orders 文本 → `certificate.json` |

这四个程序都从当前目录读 `instance.txt`（格式同上），写 `orders.txt`。
日志见 `logs/p009/`：早期最好结果是 **2006**（`ls_sa201.log`：
`initial makespan = 2006`，600 s、25,630,562 次迭代后仍是 `best makespan = 2006`），
没有一份达到 2005。本轮结论是"未突破，不提交 shot"。
