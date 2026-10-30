#!/usr/bin/env python3
"""p010 · 从官方 verify.py 提取 Mona Lisa TSP 的坐标与基线巡回

用法:
    python3 p010/extract_instance.py [--verify <verify.py>] [--out <dir>]

官方验证器把 100,000 个坐标以 zlib+base85 内嵌在 verify.py 里（并用 sha256
校验）。本脚本直接调用它的 `coordinates()`，所以拿到的数据与验证器逐字节一致。

输出:
    <out>/coords.txt        每行 "x y"（0-based 城市号 = 行号）
    <out>/baseline_tour.txt 每行一个 0-based 城市号，长度 5,757,191 的那条巡回

默认会去常见的相对路径找 few-shots 仓库；找不到就报错并提示用 --verify 指定。
"""
import argparse
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parents[1]


def find_verify() -> Path:
    cands = [
        HERE.parent / "few-shots" / "problems" / "p010-mona-lisa-tsp" / "verify" / "verify.py",
        HERE.parent.parent / "few-shots" / "problems" / "p010-mona-lisa-tsp" / "verify" / "verify.py",
    ]
    for c in cands:
        if c.is_file():
            return c
    raise SystemExit("找不到 few-shots 的 verify.py，请用 --verify 指定")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--verify", type=Path, default=None)
    ap.add_argument("--out", type=Path, default=HERE / "data" / "p010")
    args = ap.parse_args()

    verify = args.verify or find_verify()
    spec = importlib.util.spec_from_file_location("p010_verify", verify)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)

    xs, ys = mod.coordinates()
    print(f"cities = {len(xs)}   x range [{min(xs)}, {max(xs)}]   y range [{min(ys)}, {max(ys)}]")

    args.out.mkdir(parents=True, exist_ok=True)
    coords = args.out / "coords.txt"
    with open(coords, "w") as f:
        for x, y in zip(xs, ys):
            f.write(f"{x} {y}\n")
    print(f"wrote {coords}")

    base_cert = verify.parents[1] / "baseline" / "certificate.json"
    if not base_cert.is_file():
        raise SystemExit(f"找不到基线证书 {base_cert}")
    tour = json.load(open(base_cert))["tour"]
    assert sorted(tour) == list(range(1, len(xs) + 1)), "baseline tour is not a permutation of 1..N"

    out_tour = args.out / "baseline_tour.txt"
    with open(out_tour, "w") as f:
        f.write("".join(f"{c - 1}\n" for c in tour))
    print(f"wrote {out_tour}  ({len(tour)} cities, 1-based → 0-based)")

    # 复核长度
    def dst(i, j):
        dx, dy = xs[i] - xs[j], ys[i] - ys[j]
        return mod.euc_2d(dx, dy)

    total = sum(dst(tour[i] - 1, tour[(i + 1) % len(tour)] - 1) for i in range(len(tour)))
    print(f"baseline tour length = {total}   (expected 5757191)")
    return 0 if total == 5757191 else 1


if __name__ == "__main__":
    sys.exit(main())
