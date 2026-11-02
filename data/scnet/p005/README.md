# p005 数据

## `cap6_best.json`

85 个整数的列表，是本地搜索在 F₃⁶ 中找到的最大帽集（乘积路线的子问题）。
每个整数按 **6 位三进制、高位在前** 展开就是一个 F₃⁶ 点。

复核（帽集 ⇔ 没有三点共线）：

```python
import json, itertools
raw = json.load(open('cap6_best.json'))
def digs(i, d=6):
    p = [0]*d
    for k in range(d-1, -1, -1):
        p[k] = i % 3; i //= 3
    return tuple(p)
S = {digs(i) for i in raw}
third = lambda a, b: tuple((-(x+y)) % 3 for x, y in zip(a, b))
print(len(S), sum(1 for a, b in itertools.combinations(S, 2) if third(a, b) in S))
# 85 0
```

**这不是 112 点帽集。** α(F₃⁶) = 112 是已知可达的值，本地只搜到 85，说明子搜索
不够强。不能拿这个 85 去论证乘积路线不可行——那会把“搜索失败”写成“不存在”。
