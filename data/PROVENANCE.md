# 数据来源与授权 · Provenance

本目录下的实例数据与基线解**不是我的作品**，全部来自 `few-shots` 仓库及其上游来源。`few-shots` 的授权是：

- **代码**：Apache License 2.0（`LICENSE`）
- **数据**：**CC0 1.0 Universal**（`LICENSE-DATA`，公有领域贡献）

因此本目录按 CC0 原样分发，仅作可复现性用途。若上游后续变更授权，以 `few-shots` 仓库为准。

| 文件 | 内容 | 上游来源 |
|---|---|---|
| `p005/baseline_pts.txt` | F₃⁸ 中 512 个点的 cap set（每行 8 个 0/1/2） | Romera-Paredes et al., *Nature* 625 (2024) 的 `cap_set/n8_size512.txt`，经 few-shots `baseline/certificate.json` 转出 |
| `p006/baseline.seq` | LABS N=100 的基线序列 | few-shots `p006-labs-100/baseline/` |
| `p008/baseline_106.json` | AES S 盒 106 门电路（34 AND / 69 XOR / 3 XNOR，深度 24） | Jain (2026), `gf2m-circuits/sbox/`，经 few-shots 转换 |
| `p008/best_known_102.json` | AES S 盒 102 门电路（33 AND / 63 XOR / 6 XNOR） | few-shots 参与者 **huangxin-design** 的 shot `aes102-field-plane-offsets` |
| `p009/instance.txt` | 作业车间 ta41：30 工件 × 20 机器（每行 `机器 工时`） | Taillard (1993) 的 JOB-SHOP 基准，经 few-shots 内嵌 |
| `p009/baseline_orders.txt` | ta41 的 2006 基线调度（20 行 × 30 个工件号） | Shylo & Pardalos (2008)，经 few-shots `baseline/certificate.json` 转出 |
| `p010/coords.txt` | Mona Lisa TSP 的 100,000 个整数坐标 | Bosch (2009) / Cook 的 TSP 页面，经 few-shots `verify/verify.py` 内嵌数据解出 |
| `p010/baseline_tour.txt` | 长度 5,757,191 的巡回（0-based 城市号） | Nagata (2009)，经 few-shots `baseline/certificate.json` 转出（1-based → 0-based） |

## 关于 `best_known_102.json`

这份 102 门电路是 few-shots 参与者 **huangxin-design** 提交并已合并的 shot，不是我的成果。我把它放进仓库只为了让他人能复跑我的电路体检脚本（`p008/check.py`）。若作者希望撤回，删掉该文件即可，脚本也可指向任意自备证书。

## 我不主张的权利

- 实例数据、基线解、他人 shot 的著作权/相关权利归原作者。
- `docs/REPORT.md` 的文字与 `p00*/`、`tools/` 下的代码是我（Sheyuy）写的，按 MIT / CC BY 4.0 授权。
