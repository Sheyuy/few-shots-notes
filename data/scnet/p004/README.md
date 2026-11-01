# p004 数据集

五个点集，每个都是 C7<sup>⊠10</sup> 的独立集，每行一个 10 位 base-7 字符串
（前 5 位是 prefix，后 5 位是 suffix）。全部经官方 verifier 复算，**全部是极大集**
（没有可加点，`m0 = 0`）。

| 文件 | 点数 | canonical_hash | 来源 |
| --- | ---: | --- | --- |
| `cc_10_7_134753_paper.txt` | 134,753 | `650b05567deb…` | 逐字复现论文构造（`src/scnet/p004/paper_construct.py`） |
| `cc_10_7_134753_rearranged.txt` | 134,753 | `fe928a1cb18c…` | 用 3,232 个等大小替换把基线改成另一个极大集（`lns4.c` 的批量重排） |
| `cc_10_7_134753_9pair17.txt` | 134,753 | `1e69296d58d1…` | 九对族最优解：134,770 删 17 个槽位（`frontier9.py`） |
| `cc_10_7_134753_r1c19.txt` | 134,753 | `d2172281e517…` | R1 非对称族第 19 类的产出（`emit_class.py`） |
| `cc_10_7_134753_r2c3.txt` | 134,753 | `4dda701170fb…` | R2 非对称族第 3 类的产出（`emit_class.py`） |

五个集合互不相同，但都恰好 134,753 点，都不是 134,754。

## 极大性（没有可加点）

| 文件 | `m0` | `m1` |
| --- | ---: | ---: |
| paper | 0 | 5,178 |
| rearranged | 0 | — |
| 9pair17 | 0 | 5,762 |
| r1c19 | 0 | 5,766 |
| r2c3 | 0 | 5,747 |

`m0` 是“可以直接加进去的点数”，用 `sixm3 --max-k 1` 复算：

```bash
g++ -x c++ -std=c++17 -O3 -I/tmp/ompstub sixm3.c /tmp/ompstub/omp.c -o sixm3   # 无 libomp 时
./sixm3 --input cc_10_7_134753_r1c19.txt --max-k 1 --threads 1 --seconds 120
# COUNTS m0=0 m1=5766 ... m6=3160870
```

这一步是必要的：verifier 只检查“独立”，不检查“极大”。构建出来的 134,753 点集
完全可能还留着一个空位——那会直接是 134,754。本轮把五个集合都补查了，`m0` 全是 0。

## 校验

```bash
export FEW_SHOTS_REPO=/path/to/few-shots
python3.13 verify_points.py cc_10_7_134753_paper.txt   # 需要 3.11+（官方 verifier 用 tomllib）
# {"valid":true,"score":"134753","canonical_hash":"650b0556...","message":"ok"}
```

`MANIFEST.sha256` 是本目录 txt 文件的 SHA-256（**文件字节**），
和 verifier 的 `canonical_hash`（点集内容）不是一回事，两者都在。

## 复现 r1c19 / r2c3

```bash
python3 src/scnet/p004/emit_class.py \
    --checkpoint data/scnet/p004/asym_r1_checkpoint.json --index 18 \
    --rep-old 6,0,6,4,5 --rep-new 6,1,6,5,5 --out /tmp/r1c19.txt
```

CP-SAT 的最优解不唯一：同一类重跑可能得到另一个同样合法、同样 134,753 点的集合，
canonical hash 会变。`asym_r1_checkpoint.json` / `asym_r2_checkpoint.json` 是两个族的完整检查点
（各 243 个状态，`frontier=0`）；`sweep.py` 里内嵌的是 R2 族的，R1 的用
`--rep-old 6,0,6,4,5 --rep-new 6,1,6,5,5` 配 `emit_class.py` 读。

## 说明

- 9pair17 的 17 是**该固定九对族的最小删点数**（CP-SAT 证明 OPTIMAL），
  不是“整个 p004 至少需要删 17 点”的结论。
- rearranged 集合的 `m1` 没有单独记录；它的 `m0 = 0` 经过检验，k ≤ 5 的交换排除也做过。
- 所有文件的点均为 0–6 的数字串，不含坐标分隔符；`verify_points.py` 会去掉空白，
  因此空格分隔的原始格式也能直接读。
