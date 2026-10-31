# p004 数据集（2026-10-31 轮次）

三个点集，每个都是 C7<sup>⊠10</sup> 的独立集，每行一个 10 位 base-7 字符串
（前 5 位是 prefix，后 5 位是 suffix）。全部经官方 verifier 复算。

| 文件 | 点数 | canonical_hash | 来源 |
| --- | ---: | --- | --- |
| `cc_10_7_134753_paper.txt` | 134,753 | `650b05567deb26a339a788674dce5a36cba3beaeafe2c6daae631bed481b0df8` | 逐字复现论文构造（`src/scnet/p004/paper_construct.py`） |
| `cc_10_7_134753_rearranged.txt` | 134,753 | `fe928a1cb18ca8eb37dc50b1fd2df41a130db85bb4477fae2c2d390af4ff32f8` | 用 3,232 个等大小替换把基线改成另一个极大集（`lns4.c` 的批量重排步骤） |
| `cc_10_7_134753_9pair17.txt` | 134,753 | `1e69296d58d184ac1d554d723df28763d5cfc8dcf74d7bdc20a4c15242a205ec` | 九对族的最优解：358² + 18·367 = 134,770，删 17 个槽位后 134,753（`frontier9.py`） |

三个集合互不相同，但都恰好 134,753 点，都不是 134,754。

## 校验

```bash
export FEW_SHOTS_REPO=/path/to/few-shots
python3 verify_points.py cc_10_7_134753_paper.txt
# {"valid":true,"score":"134753","canonical_hash":"650b0556...","message":"ok"}
```

`MANIFEST.sha256` 是本目录 txt 文件的 SHA-256（不是 verifier 的 canonical hash）。

## 说明

- 9pair17 的 17 是**该固定九对族的最小删点数**（CP-SAT 证明 OPTIMAL），
  不是“整个 p004 至少需要删 17 点”的结论；最优解不唯一，重跑
  `frontier9.py --write` 可能得到另一个同样 17 空位、同样 134,753 点的集合。
- rearranged 集合只有“无法直接加点”这一性质经过检验；它同样通过了
  k ≤ 5 的交换排除（见 `src/scnet/p004/README.md`）。
- 所有文件的点均为 0–6 的数字串，不含坐标分隔符；`verify_points.py` 会去掉空白，
  因此空格分隔的原始格式也能直接读。
