# Few Shots 开放题攻坚笔记

对 [few-shots](https://github.com/few-shots/few-shots) 开放题的一次系统性攻坚记录：**5 道题、14 节完整报告、约 16 万字节源码**。

先把结论放在最前面，免得你误以为这里有"突破"：

> **5 道题全部没有突破，世界纪录保持不变；因此没有创建任何 shot、没有开过 PR。**
>
> 这个仓库真正的价值不是"我赢了"，而是**"为什么打不动"的证据链**，以及一套可复用的诊断/搜索工具。

---

## 一、战况总表

| 题目 | 纪录 | 突破线 | 我做了什么 | 结论 |
|---|---|---|---|---|
| **p005** 8 维帽集 F₃⁸ | 512 | ≥ 513 | 分层化简 + 24 组单层替换 + 精确命中集 + k-紧致性 | 512 是极深局部最优；阶段 0 线性层**可证明最优** |
| **p006** LABS N=100 | 578 | ≤ 577 | 模拟退火 + 迭代局部搜索（2492 次重启） | 578 是 E≡2 mod 4 的孤立极小 |
| **p008** AES S 盒电路 | 102 | ≤ 101 | 5 类通用再综合 + 线性层 SLP 拆解 | 102 在通用变换下紧致；阶段 0 线性层**可证明最优** |
| **p009** 作业车间 ta41 | 2006 | ≤ 2005 | 禁忌 / IG / SA（插入 + N5/N6）+ HGA + 分布式 | 约 **9.9 亿次迭代**全部止于 2006，判定即最优 |
| **p010** Mona Lisa TSP 10 万城 | 5,757,191 | ≤ 5,757,190 | 2-opt / Or-opt(≤25) / 3-opt 扫描 + chained LK | 三级精确最优 + **40.6 万次扰动**无改进 |

另有 **p007**（汤姆孙 N=960）经调研确认已被两支队伍推到 `444368.3924` 且列尽死路，本轮未重复投入；**p001/p002/p004** 为公开难题，**p003** 可能物理不可行。

---

## 二、三个最有价值的结论

### 1. p005：初始线性层 18 门可证明最优
把每条 wire 写成"生成元（x₀..x₇ + 33 个 AND 输出）上的 42 位仿射型"后，电路分解为三块线性层。其中喂前 9 个 AND 的初始线性块有 **18 个互异目标型**（权 5–7，都不是输入）：

> 每个新的 affine form 都必须由一个新门产生 → **至少 18 门**；而电路正好用 18 门 → **最优，没有节省空间**。

### 2. p010：巡回是 3-opt 最优，且对 4-opt 双桥扰动稳定
- 2-opt、Or-opt（段长 1–25）、完整 3-opt（7 种重连，组合枚举推出）三级扫描：**0 个改进移动**。
- 扫描器自检：把巡回随机反转 2001 城（长度劣化到 5,768,149），扫描器立刻报出 **478** 个改进 → "0" 不是失灵。
- 实现 3-opt 的 **apply** + DLB 修复 + 双桥扰动：**406,378 次独立扰动，0 次改进**，全部精确停回 5,757,191。

### 3. p009：9.9 亿次迭代的"深吸引子"证据
所有配置（禁忌/IG/SA × 插入/N5N6 邻域 + 6 进程分布式）的 `best` 都停在 2006，而且官方验证器给出 **canonical_hash 与 baseline 逐字节相同**——搜索不是"接近"了 2006，而是**精确收敛回同一条调度**。这是"2006 即最优"最强的经验证据。

---

## 三、仓库结构

```
.
├── README.md
├── LICENSE
├── docs/
│   └── REPORT.md          # 14 节完整报告（过程、数据、踩坑、结论）
├── p005/                  # 帽集：分层扫描 / 精确命中集 / 强 LNS / 随机游走
├── p006/                  # LABS：SA + ILS
├── p008/                  # AES 电路：体检器 + 线性层 SLP 分析
├── p009/                  # 作业车间：禁忌 / IG / SA / HGA / 证书转换
├── p010/                  # TSP：2-opt/Or-opt/3-opt 扫描 + chained LK
└── tools/
    └── make_shot.py       # 一旦突破，自动生成合规 shot 目录并自检
```

---

## 四、如何复现

本仓库**不包含**任何第三方数据（实例文件、基线证书），只包含代码。复现需要先准备 `few-shots` 仓库与实例数据：

```bash
git clone https://github.com/few-shots/few-shots
```

### p010（Mona Lisa TSP）
坐标内嵌在官方 `verify/verify.py` 里（zlib + base85）。提取成 `coords.txt` / `baseline_tour.txt`：

```python
import importlib.util, json
spec = importlib.util.spec_from_file_location(
    "v", "few-shots/problems/p010-mona-lisa-tsp/verify/verify.py")
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
xs, ys = m.coordinates()
open("coords.txt","w").write("".join(f"{x} {y}\n" for x,y in zip(xs,ys)))
t = json.load(open("few-shots/problems/p010-mona-lisa-tsp/baseline/certificate.json"))["tour"]
open("baseline_tour.txt","w").write("".join(f"{c-1}\n" for c in t))
```

```bash
cc -O2 -o tsp_3opt  tsp_3opt.c  -lm && ./tsp_3opt        # 3-opt 最优性诊断
cc -O2 -o tsp_ils_dlb tsp_ils_dlb.c -lm
./tsp_ils_dlb 600 201 1000 1 100                          # 600s, seed, gap, 桥数, travel
```

### p009（作业车间 ta41）
需要 `instance.txt` 与 `baseline_orders.txt`（ta41 的 30×20 数据与基线机器序）。编译后：

```bash
cc -O2 -o jobshop_sa jobshop_sa.c -lm
./jobshop_sa 1800 101 best.txt 5 150 0.05   # 秒, seed, 输出, kick, T0, Tend
```

### p008（AES S 盒电路）
直接读证书 JSON，无需额外数据：

```bash
python3 p008/check.py few-shots/problems/p008-aes-sbox-circuit/shots/<...>/certificate.json
python3 p008/check.py <cert> --walk 60      # 额外跑 60s 随机再综合，找死线
python3 p008/slp.py <cert>                  # 线性层拆解 + SLP 下界
```

### p005（帽集）
需要把 baseline 证书导成 `baseline_pts.txt`（每行 8 个 0/1/2），然后 `./cap_layer_lns`。

---

## 五、方法论（比结果更值得复用的部分）

1. **先证局部最优，再谈搜索。** 盲目加算力是最贵的错误。p005 用穷举证明"单点插入邻域 0 个改进"，p010 用三级扫描证明 2/3-opt 最优——这直接解释了此前所有搜索为什么冻结。
2. **每次改动都自校验。** 所有局部搜索在 apply 之后重算目标值；扫描器用"人为劣化 + 能否检出"来自证没坏。
3. **精确优于启发。** p005 用命中集 IDDFS 把 `maxcap(R)` 算到精确值（192），才敢断言"这个 B 已经最优"。
4. **分布式 + 参数扫描。** p009 的 6 进程并行（不同种子/温度/kick）、p010 的 8 组配置，避免单点运气。
5. **文献核查优先于重复劳动。** 先确认公开纪录是否已被刷新，能省掉大量无用搜索。

---

## 六、踩过的坑（都是真的）

| 坑 | 现象 | 根因 |
|---|---|---|
| 2-opt 侧向公式 | 报出 1992 个"改进"，gain 最大 58 | `side=1`（前驱方向）用了错误的交叉重连公式 |
| 3-opt gain 符号 | 报出 4390 万个改进，gain 3404 | 写成 `new − cut`，应为 `cut − new`；gain 3404 会把长度压到下界 5757084 以下，物理不可能——正是这个矛盾暴露了 bug |
| HGA 交叉 | ASan: stack-buffer-overflow | 单调指针把"暂未就绪"的工件永久跳过 → 某步无候选 → `best_m = -1` → `child[-1]` |
| zsh 不拆词 | SA 首轮冻死（T0=0, kick_d=0） | `set -- $params` 在 zsh 下不按空格拆分 |
| insert_job 非幂等 | SIGBUS / len[m] 膨胀到 34 | 工件未真正移除就被重复插入，数组越界 |
| z3 编码过重 | 连已知 SAT 的 24 门情形都 unknown | 用 Array + 选择变量编码 SLP，老版本 z3 啃不动 |

---

## 七、为什么没有 PR

few-shots 的规则是：**必须严格优于纪录（ε 以内）才算一次 shot**。五题的最好成绩都只是"追平纪录"，不构成突破，所以：

- 没有创建 `shots/<login>/<shot-id>/`
- 没有 push 到 few-shots
- 没有开 PR

这是遵守规则，不是放弃。

---

## License

代码 MIT；`docs/REPORT.md` 文字部分 CC BY 4.0。

第三方数据（题目实例、基线证书）版权归原作者，本仓库不包含，仅注明来源。
