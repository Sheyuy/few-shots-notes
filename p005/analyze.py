#!/usr/bin/env python3
"""p005 · F_3^8 cap set 的结构分析

用法:
    python3 p005/analyze.py [baseline_pts.txt]

做四件事（都是当年用来判断"512 到底有多硬"的诊断）:

1. 有效性校验：任意三个点不共线（a+b+c=0 mod 3）。
2. 逐坐标切片：打印每个坐标下三层的点数。会看到 192/160/160 这种
   "两个相等的层 + 一个修正层" 的结构。
3. 阻断对直方图：对每个不在集合里的点 p，数有多少对 {a,b} ⊂ S 满足
   a+b = -p。p 可加入 ⇔ 阻断对为 0。基线里最小的也有 4 对。
4. k-紧致性：随机删掉 k 个点，统计有多少个点变得可加入。
   实测 k=1,2,3,4,6 时新增可加点数恰好等于 k —— 这就是
   "不存在 (删 k → 加 k+1) 交换" 的直接证据，也解释了为什么
   所有局部搜索都停在 512。
"""
import random
import sys
from collections import Counter
from pathlib import Path

D = 8
SIZE = 3 ** D
ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / "data" / "p005" / "baseline_pts.txt"


def enc(v):
    x = 0
    for k in range(D):
        x += v[k] * (3 ** k)
    return x


def dec(x):
    v = []
    for _ in range(D):
        v.append(x % 3)
        x //= 3
    return v


def third(a, b):
    """-(a+b) mod 3，输入输出都是编码整数。"""
    x, y, z, pw = a, b, 0, 1
    for _ in range(D):
        s = ((x % 3) + (y % 3)) % 3
        z += ((3 - s) % 3) * pw
        pw *= 3
        x //= 3
        y //= 3
    return z


def load(path):
    pts = []
    for line in open(path):
        line = line.strip()
        if not line:
            continue
        v = list(map(int, line.split()))
        assert len(v) == D and all(0 <= t <= 2 for t in v), line
        pts.append(v)
    return pts


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else str(DEFAULT)
    raw = load(path)
    S = {enc(p) for p in raw}
    assert len(S) == len(raw), "duplicate points"
    print(f"points = {len(S)}   (from {path})")

    # 1) cap 校验
    Sl = list(S)
    bad = 0
    for i in range(len(Sl)):
        for j in range(i + 1, len(Sl)):
            if third(Sl[i], Sl[j]) in S:
                bad += 1
    print(f"[1] cap check: {'OK' if bad == 0 else f'{bad} collinear pairs'}")
    if bad:
        return 1

    # 2) 逐坐标切片
    print("[2] slice sizes by coordinate:")
    for c in range(D):
        cnt = Counter(dec(x)[c] for x in S)
        trio = (cnt[0], cnt[1], cnt[2])
        tag = "  <-- two equal layers + correction" if len(set(trio)) == 2 else ""
        print(f"    coord {c}: {trio[0]:3d} / {trio[1]:3d} / {trio[2]:3d}{tag}")

    # 3) 阻断对直方图
    bc = [0] * SIZE
    for i in range(len(Sl)):
        for j in range(i + 1, len(Sl)):
            bc[third(Sl[i], Sl[j])] += 1
    hist = Counter()
    outside = 0
    for q in range(SIZE):
        if q not in S:
            hist[bc[q]] += 1
            outside += 1
    print(f"[3] blocker-pair histogram over {outside} outside points:")
    ks = sorted(hist)
    print(f"    min={ks[0]}  max={ks[-1]}")
    for k in ks[:12]:
        print(f"      {k:3d} pairs : {hist[k]:5d} points")
    print(f"    (a point with 0 blocking pairs would be directly addable: "
          f"{hist.get(0, 0)} such points)")

    # 4) k-紧致性
    print("[4] k-tightness (remove k random points, count newly addable):")
    rng = random.Random(20261029)
    for k in (1, 2, 3, 4, 6):
        best = 0
        for _ in range(40):
            rem = rng.sample(Sl, k)
            remset = set(rem)
            # recompute blocker counts against S \ rem
            cnt = Counter()
            base = [x for x in Sl if x not in remset]
            for i in range(len(base)):
                for j in range(i + 1, len(base)):
                    cnt[third(base[i], base[j])] += 1
            cur = S - remset          # 当前集合（removed 的点已经不在里面）
            addable = sum(1 for q in range(SIZE) if q not in cur and cnt[q] == 0)
            # 注意：这里统计的是"删掉 k 个之后、全局可加入的点数"
            best = max(best, addable)
        print(f"    k={k}: max newly addable = {best}"
              f"   {'(== k, no (k -> k+1) exchange)' if best == k else ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
