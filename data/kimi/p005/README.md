# p005 / Kimi 轮次数据说明

本目录所有 `*.txt` 点集（`projective/` 除外）均采用与官方证书一致的**大端三进制**编码：

```text
id(p0,...,p7) = 3^7*p0 + 3^6*p1 + ... + p7，pi ∈ {0,1,2}
```

每行一个整数；空行和以 `#` 开头的行是注释。该顺序与 `src/codearts/p005/capset.py` 一致；射影衍生的原始点文件已统一做坐标逆序。转换和检查见 `src/kimi/p005/caplib.py`。

## 文件

- `baseline512.txt`：官方 512 点基线转成上述整数编码；官方 verifier 复验见 `logs/kimi/p005/official-baseline-verify.log`。
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
