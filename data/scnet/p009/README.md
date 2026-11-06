# p009 数据集（TA41，30×20 job shop）

| 文件 | 内容 |
| --- | --- |
| `instance.txt` | 官方实例：首行 `30 20`，随后 30 行 × 20 组 `机器 时长`，`src/scnet/p009/jssp.c` 与 `p009/*.c` 都读这个格式 |
| `instance.json` | 同一实例的 JSON 版（机器/时长数组） |
| `orders.txt` | 本轮最好的初始调度（20 行 × 30 个工序号），makespan **2006**；作为 `jssp.c` 的起点 |
| `baseline_orders.json` | 上述 orders 的 JSON 版 |

基线纪录是 **2005**，本轮所有方法（SA / 禁忌 / 迭代贪心 / HGA / 单文件 `jssp.c`）
都停在 2006，没有突破，因此没有提交 shot。相关日志在 `logs/p009/`。

```bash
cd data/scnet/p009 && ../../../../src/scnet/p009/jssp   # 需要先编译，见 src/scnet/p009/README.md
# baseline makespan 2006
```
