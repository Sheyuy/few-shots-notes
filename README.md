# Few Shots 开放题攻坚笔记

对 [few-shots](https://github.com/few-shots/few-shots) 开放题的一次系统性攻坚：**5 道题 + 1 题调研、14 节完整报告、全部源码、全部运行日志、全部实例数据**。

先把结论放在最前面，免得你误以为这里有"突破"：

> **5 道题全部没有突破，世界纪录保持不变；因此没有创建任何 shot、没有开过 PR。**
>
> 这个仓库真正的价值不是"我赢了"，而是**"为什么打不动"的证据链**，以及一套可复用的诊断/搜索工具。

---

## 一、战况总表

| 题目 | 纪录 | 突破线 | 我做了什么 | 结论 |
|---|---|---|---|---|
| **p005** 8 维帽集 F₃⁸ | 512 | ≥ 513 | 分层化简 + 24 组单层替换 + 精确命中集 + k-紧致性 | 512 是极深局部最优；阶段 0 线性层**可证明最优** |
| **p006** LABS N=100 | 578 | ≤ 577 | 模拟退火 + 迭代局部搜索（2426 次重启） | 578 是 E≡2 mod 4 的孤立极小 |
| **p008** AES S 盒电路 | 102 | ≤ 101 | 5 类通用再综合 + 线性层 SLP 拆解 | 102 在通用变换下紧致；阶段 0 线性层**可证明最优** |
| **p009** 作业车间 ta41 | 2006 | ≤ 2005 | 禁忌 / IG / SA（插入 + N5/N6）+ HGA + 分布式 | 约 **9.9 亿次迭代**全部止于 2006，判定即最优 |
| **p010** Mona Lisa TSP 10 万城 | 5,757,191 | ≤ 5,757,190 | 2-opt / Or-opt(≤25) / 3-opt 扫描 + chained LK | 三级精确最优 + **40.6 万次扰动**无改进 |
| **p007** 汤姆孙 N=960 | 444368.3924 | 更低 1e-6 | 仅文献与纪录链调研（未写求解器） | 两支队伍已扫尽对称轨道，ε 太狠，不打 |

**p001/p002/p004** 为公开难题，**p003** 可能物理不可行，均未投入。

---

## 二、三个最有价值的结论

### 1. p005：初始线性层 18 门可证明最优
把每条 wire 写成"生成元（x₀..x₇ + 33 个 AND 输出）上的 42 位仿射型"后，电路分解为三块线性层。其中喂前 9 个 AND 的初始线性块有 **18 个互异目标型**（权 5–7，都不是输入）：

> 每个新的 affine form 都必须由一个**新门**产生 → **至少 18 门**；而电路正好用 18 门 → **最优，没有节省空间**。

同一条推理对 p005 的帽集也成立：512 的"修正层"用 192 个点填满 200 点的残余区，精确 maxcap = 192，一分不差。

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
├── LICENSE                    # 代码 MIT；报告 CC BY 4.0
├── docs/
│   └── REPORT.md              # 14 节完整报告（过程、数据、踩坑、结论）
├── data/                      # 实例与基线（CC0，见 data/PROVENANCE.md）
│   ├── PROVENANCE.md
│   ├── p005/baseline_pts.txt
│   ├── p006/baseline.seq
│   ├── p008/{baseline_106.json, best_known_102.json}
│   ├── p009/{instance.txt, baseline_orders.txt}
│   └── p010/{coords.txt, baseline_tour.txt}
├── logs/                      # 44 份原始运行日志（见下方索引）
├── results/                   # 求解产物（p009 的 33 条调度、p006 的最优序列）
├── p005/                      # 8 个 C 程序 + 2 个 Python 分析脚本
├── p006/                      # LABS：SA + ILS
├── p007/RESEARCH.md           # 汤姆孙调研笔记（未写求解器）
├── p008/                      # 电路体检器 + 线性层 SLP 分析
├── p009/                      # 禁忌/IG/SA/HGA + 证书转换 + 诊断
├── p010/                      # 2-opt/3-opt 扫描 + chained LK + 实例提取
└── tools/make_shot.py         # 突破后自动生成合规 shot 的自检脚本
```

---

## 四、日志索引

| 目录 | 文件 | 说明 |
|---|---|---|
| `logs/p009/` | `sa_{1,2,3,7,11,99,2026,42}.log` | 第一轮 SA（纯插入邻域），每个 43–51M 迭代，全部 best=2006 |
| `logs/p009/` | `sa2_{101..108}.log` | 第二轮 SA（加入 N5/N6 关键块邻域），每个 66–79M 迭代，全部 best=2006 |
| `logs/p009/` | `ls_sa{201..204}.log` | 分布式 4 进程 SA（变种子/温度/kick），合计 1.02 亿迭代 |
| `logs/p009/` | `ls_hga{301,302}.log` | HGA（G&T 交叉 + SA 强化），停在 2472 / 2567 |
| `logs/p009/` | `bg{A,B,C,IG,IGd}*.log` | 早期禁忌 / IG 的批量运行 |
| `logs/p010/` | `ils_{201..208}.log` | chained LK：8 组配置（gap 1k–20k、桥数 1–2、travel 100–500），合计 406,378 次扰动 |
| `logs/p005/` | `bg.log` | 帽集删点-重填局部搜索 |
| `logs/p006/` | `bg_p006.log` | LABS SA/ILS |

---

## 五、如何复现

**数据已经随仓库提供**（`data/`，CC0），所以大部分实验可以直接跑。

```bash
# p005：结构分析（切片 / 阻断对 / k-紧致性）
python3 p005/analyze.py

# p005：强 LNS 单层替换（24 组，全部无改进）
cc -O2 -o cap_layer_lns p005/cap_layer_lns.c && ./cap_layer_lns 100 5000

# p008：任意候选电路的一键体检（正确性 + 四类重写 + 随机 walk）
python3 p008/check.py data/p008/best_known_102.json
python3 p008/check.py data/p008/best_known_102.json --walk 60

# p008：线性层拆解与 SLP 下界
python3 p008/slp.py data/p008/best_known_102.json

# p009：SA（600 秒，seed 201，输出，kick=5，T0=150，Tend=0.05）
cc -O2 -o jobshop_sa p009/jobshop_sa.c -lm && ./jobshop_sa 600 201 out.txt 5 150 0.05

# p010：3-opt 最优性诊断（0 个改进）
cc -O2 -o tsp_3opt p010/tsp_3opt.c -lm && ./tsp_3opt

# p010：chained LK（600 秒，seed，gap，桥数，travel）
cc -O2 -o tsp_ils_dlb p010/tsp_ils_dlb.c -lm && ./tsp_ils_dlb 600 201 1000 1 100
```

> `p010` 的 C 程序默认读当前目录下的 `coords.txt` / `baseline_tour.txt`，先从 `data/p010/` 拷过去，或用 `python3 p010/extract_instance.py` 从官方 `verify.py` 重新生成（会复核长度 5,757,191）。

要验证"是否构成一次 shot"，仍需 few-shots 仓库里的官方 `verify.py`：

```bash
python3 few-shots/problems/p010-mona-lisa-tsp/verify/verify.py <shot_dir>
```

---

## 六、方法论（比结果更值得复用的部分）

1. **先证局部最优，再谈搜索。** 盲目加算力是最贵的错误。p005 用穷举证明"单点插入邻域 0 个改进"，p010 用三级扫描证明 2/3-opt 最优——这直接解释了此前所有搜索为什么冻结。
2. **每次改动都自校验。** 所有局部搜索在 apply 之后重算目标值；扫描器用"人为劣化 + 能否检出"来自证没坏。
3. **精确优于启发。** p005 用命中集 IDDFS 把 `maxcap(R)` 算到精确值（192），才敢断言"这个 B 已经最优"。
4. **分布式 + 参数扫描。** p009 的 6 进程并行（不同种子/温度/kick）、p010 的 8 组配置，避免单点运气。
5. **文献核查优先于重复劳动。** 先确认公开纪录是否已被刷新，能省掉大量无用搜索（p007 就是靠这一步直接放弃的）。

---

## 七、踩过的坑（都是真的）

| 坑 | 现象 | 根因 |
|---|---|---|
| 2-opt 侧向公式 | 报出 1992 个"改进"，gain 最大 58 | `side=1`（前驱方向）用了错误的交叉重连公式 |
| 3-opt gain 符号 | 报出 4390 万个改进，gain 3404 | 写成 `new − cut`，应为 `cut − new`；gain 3404 会把长度压到下界 5757084 以下，物理不可能——正是这个矛盾暴露了 bug |
| HGA 交叉 | ASan: stack-buffer-overflow | 单调指针把"暂未就绪"的工件永久跳过 → 某步无候选 → `best_m = -1` → `child[-1]` |
| zsh 不拆词 | SA 首轮冻死（T0=0, kick_d=0） | `set -- $params` 在 zsh 下不按空格拆分 |
| insert_job 非幂等 | SIGBUS / `len[m]` 膨胀到 34 | 工件未真正移除就被重复插入，数组越界 |
| K=12 太弱 | 双桥修复只消掉 30% 扰动 | 邻域太小；改 K=20 后多数扰动可完全修复 |
| z3 编码过重 | 连已知 SAT 的 24 门情形都 unknown | 用 Array + 选择变量编码 SLP，老版本 z3 啃不动 |

---

## 八、为什么没有 PR

few-shots 的规则是：**必须严格优于纪录（ε 以内）才算一次 shot**。五题的最好成绩都只是"追平纪录"，不构成突破，所以：

- 没有创建 `shots/<login>/<shot-id>/`
- 没有 push 到 few-shots
- 没有开 PR

这是遵守规则，不是放弃。

---

## 九、数据来源与授权

`data/` 下的实例与基线解**不是我的作品**，来自 few-shots 及其上游（Taillard 的 JOB-SHOP、Bosch/Cook 的 Mona Lisa TSP、Romera-Paredes et al. 的 512-cap、Jain 2026 的 106 门电路、参与者 huangxin-design 的 102 门电路）。few-shots 的数据以 **CC0 1.0** 分发，故本仓库同样按 CC0 原样提供，仅作可复现用途。

逐文件的来源与授权见 **[`data/PROVENANCE.md`](data/PROVENANCE.md)**。

## License

- 代码（`p00*/`、`tools/`）：MIT
- 报告文字（`docs/REPORT.md`、`p007/RESEARCH.md`）：CC BY 4.0
- 第三方数据（`data/`）：CC0 1.0（随 few-shots）
