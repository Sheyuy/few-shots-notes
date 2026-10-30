# p008 · Smallest Circuit for the AES S-box

**纪录**: 102 门（huangxin-design 2026-10-28）
**方向**: min
**本轮结果**: 102（未突破）

## 基线复现

```bash
python3 problems/p008-aes-sbox-circuit/verify/verify.py problems/p008-aes-sbox-circuit/shots/huangxin-design/aes102-field-plane-offsets
# 输出 valid:true, score:102
```

102 个门，每个门为 AND/XOR/XNOR，输入为 8 位 x0..x7，输出为 8 位 q83/q87/q92/q93/q94/q96/q99/q101。

## 实验：真值表去重（`verify_found.py`）

对 102 个门的输出线计算 256-bit 真值表，检查：
- **重复真值表门**：0 个（没有两个门输出完全相同的真值表）。
- **死门**：0 个（每个门都被输出或其他门引用）。
- **等于输入的门**：0 个。
- **常量门**：0 个。

## 结论

102 门电路**没有免费删一门**的捷径。要突破必须做结构性优化（如代数化简、公共子表达式消除、或全新的代数构造）。

## 未尝试的方向

- 用 SAT 编码小规模电路优化（固定 101 门，搜索是否存在满足约束的电路）。
- 用代数化简工具（如 PolyBoRi、SageMath）对 GF(2^8) 上的 S-box 多项式做最小电路综合。
- 尝试不同的域表示（如不同的不可约多项式）。
