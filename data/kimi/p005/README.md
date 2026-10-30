# p005 / Kimi 轮次数据说明

本目录点集采用**大端三进制**编码。除特别注明的七维文件外，均为八维点：

```text
id(p0,...,p7) = 3^7*p0 + 3^6*p1 + ... + p7，pi ∈ {0,1,2}
```

每行一个整数；空行和以 `#` 开头的行是注释。该顺序与 `src/codearts/p005/capset.py` 一致；射影衍生的原始点文件已统一做坐标逆序。转换和检查见 `src/kimi/p005/caplib.py`。

## 文件

- `cap7-236.txt`：七维 236 点帽，每点是 7 个大端三进制数字对应的整数；由 `make_cap7_236.py` 确定性生成。
- `seed8-fixed236.txt`：把上述七维帽嵌入八维第一坐标为 0 的层，仍是合法 236 点帽。
- `fixed236-near41.txt`：永久保留该 236 点层的 513 点非法近 miss，含 41 条冲突线。
- `fixed236-repaired472.txt`：禁止删除 236 个固定点时的精确最小覆盖修复结果，合法 472 点帽；最优覆盖不唯一，复跑可得到不同但等大的修复集。
- `baseline512.txt`：官方 512 点基线转成上述整数编码；官方 verifier 复验见 `logs/kimi/p005/official-baseline-verify.log`。
- `baseline512-normalized.txt`：原基线的可逆仿射像，合法 512 点帽，包含 SAT 模型固定的 9 个基点及二支撑点 id 2916。
- `baseline512-normalized-orbit12.txt`：上述规范化帽的坐标置换，合法 512 点帽，包含同一组基点及二支撑点 id 3645；用于第二 SAT 轨道的单位见证正对照。
- `baseline512-normalized-no-support2.txt`：同一原基线在另一帽内仿射基点下的规范化像，合法 512 点帽；基点外没有任何二支撑点，用于第三 SAT 分区的单位见证正对照。
- `baseline512-no-support2-basis.txt`：产生上述规范化所选的 9 个源帽点，首行为原点，其余 8 行为独立差向量；搜索重放日志见 `basis-low-support.log`。
- `baseline512-no-support2-weight3-t0.txt`：无二支撑见证的坐标置换，额外包含全 1 型三支撑点 `e1+e2+e3`（id 3159），用于更细 SAT 分支的单位见证正对照。
- `baseline512-no-support2-weight3-t1.txt`：同一无二支撑见证的另一坐标置换，额外包含 `e1+e2+2e3`（id 3402），用于第二类三支撑 SAT 分支。
- `baseline512-no-support2-weight3-t2.txt`：同一见证的坐标置换，额外包含 `e1+2e2+2e3`（id 4131），用于第三类三支撑 SAT 分支。
- `baseline512-no-support2-weight3-t3.txt`：同一见证的坐标置换，额外包含 `2e1+2e2+2e3`（id 6318），用于第四类三支撑 SAT 分支。
- `fixed192-recovered512.txt`：固定基线第一坐标为 0 的 192 点层后，从 512 点 SAT 正对照日志回读的帽；与原基线逐点一致，用于检验变量映射和回读器。
- `seeds496/seed0.txt` … `seed5.txt`：六个合法、相互不完全相同的 496 点帽，来源于射影帽的仿射截面及后续增长。它们用于逐盆地精确排除，不是一个 497 点候选。
- `nearmiss/near{29,34,22,10,7,4}.txt`：513 点的非法近 miss；文件名数字是冲突线数，不是合法帽大小。
- `repaired/`：对应近 miss 经精确点覆盖修复后的合法帽；`near22-keep-new-cap491.txt` 额外强制保留全部 7 个非基线点。
- `projective/pg10-cap2744.txt`、`pg11-cap6464.txt`、`pg12-cap13312.txt`：射影帽的生成矩阵。每行是一个坐标，每列是一个射影点；维度参数分别为 `D=11,12,13`，列数分别为 `P=2744,6464,13312`。
- `projective/*functionals.bin`：历史搜索实际使用的函数向量，行优先 `uint8`。三份分别为 88573×11、265720×12、131072×13；PG12 是采样集合。历史编号顺序并不统一，复放子空间记录时必须使用这些原文件，不能用另一种规范化顺序替代。

## 种子来源映射

| 归档名 | 原工作文件 |
| --- | --- |
| `seed0.txt` | `cap6464-affine-walk-best-0.txt` |
| `seed1.txt` | `cap6464-subspace-walk-2-best-0.txt` |
| `seed2.txt` | `cap6464-subspace-walk-3-best-0.txt` |
| `seed3.txt` | `cap6464-subspace-walk-4-best-0.txt` |
| `seed4.txt` | `cap13312-sample472-walk-best-0.txt` |
| `seed5.txt` | `cap13312-aff484-grow-best-0.txt` |

所有结论、命令和适用范围见 `docs/REPORT_KIMI_P005.md`；`MANIFEST.sha256` 固定本目录数据内容。
