#!/usr/bin/env python3
"""merge_best.py -- keep the global best certificate across runs.

Usage:  python3 merge_best.py best.json <run1.txt> <run2.txt> ...
Reads every candidate sequence file, computes E with exact integer arithmetic,
keeps the best in best.json (certificate format) and prints the best E found.
"""
import json
import os
import sys


def energy(seq: str) -> int:
    s = [1 if c == '+' else -1 for c in seq]
    n = len(s)
    total = 0
    for k in range(1, n):
        c = 0
        for i in range(n - k):
            c += s[i] * s[i + k]
        total += c * c
    return total


def read_seq(path: str):
    try:
        with open(path, 'rb') as f:
            data = f.read(65536).decode('utf-8', 'ignore')
    except OSError:
        return None
    cnt = 0
    start = -1
    for i, ch in enumerate(data):
        if ch in '+-':
            if start < 0:
                start = i
            cnt += 1
            if cnt == 100:
                return data[start:start + 100]
        else:
            cnt = 0
            start = -1
    return None


def main(argv):
    if len(argv) < 2:
        print("usage: merge_best.py best.json [candidates...]")
        return 2
    best_path = argv[1]
    best_seq = None
    best_e = None
    if os.path.exists(best_path):
        try:
            with open(best_path) as f:
                obj = json.load(f)
            cand = obj['sequence']
            if len(cand) == 100 and set(cand) <= set('+-'):
                best_seq = cand
                best_e = energy(cand)
        except Exception:
            pass
    for path in argv[2:]:
        seq = read_seq(path)
        if not seq:
            continue
        e = energy(seq)
        if best_e is None or e < best_e:
            best_e = e
            best_seq = seq
    if best_seq is None:
        print("no candidate")
        return 1
    if best_e % 4 != 2:
        print("WARNING: E %d is not 2 (mod 4)" % best_e)
    with open(best_path, 'w') as f:
        json.dump({"sequence": best_seq}, f, separators=(",", ":"))
    print("best E=%d" % best_e)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
