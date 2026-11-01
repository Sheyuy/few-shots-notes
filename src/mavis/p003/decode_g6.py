#!/usr/bin/env python3
"""Decode Exoo's r46_35some.g6 (37 lines of 35-vertex graph6) and score each
line as a (4,6)-Ramsey graph: count K4 cliques and independent 6-sets.

The graph6 format (McKay's spec):
  byte 0  : n + 63
  rest    : 6-bit groups, MSB first, listing edges (0,1),(0,2),(1,2),(0,3),...
             i.e. for j = 1..n-1, for i = 0..j-1

Scoring is bitmask-based so 37 graphs x (C(35,4) + C(35,6)) is instant.

Usage: python3 decode_g6.py <file.g6>
"""
import json
import sys


def parse_g6(line):
    s = line.strip()
    if not s:
        return None
    n = ord(s[0]) - 63
    if n < 0 or n > 63:
        raise ValueError('bad header')
    need = (n * (n - 1) // 2 + 5) // 6
    body = s[1:]
    if len(body) < need:
        raise ValueError(f'truncated: need {need} got {len(body)}')
    bits = []
    for ch in body[:need]:
        v = ord(ch) - 63
        for k in range(5, -1, -1):
            bits.append((v >> k) & 1)
    adj = [0] * n                       # bitmask adjacency
    idx = 0
    for j in range(1, n):
        for i in range(j):
            if bits[idx]:
                adj[i] |= 1 << j
                adj[j] |= 1 << i
            idx += 1
    return adj


def score(adj):
    """Return (n, K4, I6, degmin, degmax)."""
    n = len(adj)
    full = (1 << n) - 1

    # --- K4: for each edge (i,j) with i<j, count adjacent pairs among common nbrs
    k4 = 0
    for j in range(n):
        gt = ~((1 << (j + 1)) - 1) & full        # vertices > j
        for i in range(j):
            if (adj[i] >> j) & 1:
                c = adj[i] & adj[j] & gt
                while c:
                    low = c & -c
                    v = low.bit_length() - 1
                    c ^= low
                    k4 += bin(c & adj[v]).count('1')

    # --- I6: independent sets of size 6 via mask recursion (prunes hard)
    nonadj = [full & ~adj[i] & ~(1 << i) for i in range(n)]

    def count(mask, size):
        if size == 0:
            return 1
        total = 0
        while mask:
            low = mask & -mask
            v = low.bit_length() - 1
            mask ^= low
            total += count(mask & nonadj[v], size - 1)
        return total

    i6 = count(full, 6)
    degs = [bin(a).count('1') for a in adj]
    return n, k4, i6, min(degs), max(degs)


def main():
    path = sys.argv[1]
    lines = [l for l in open(path, errors='replace') if l.strip()]
    print(f'{path}: {len(lines)} lines', flush=True)
    valid = []
    for idx, line in enumerate(lines):
        try:
            adj = parse_g6(line)
            n, k4, i6, dmin, dmax = score(adj)
        except Exception as e:
            print(f'{idx}: FAIL {e}', flush=True)
            continue
        flag = '  <== VALID (4,6)-graph' if (k4 == 0 and i6 == 0) else ''
        print(f'{idx}: n={n} K4={k4} I6={i6} deg=[{dmin},{dmax}]{flag}', flush=True)
        if k4 == 0 and i6 == 0:
            valid.append(idx)
    print(f'--- valid (4,6)-graphs: {len(valid)} -> {valid}', flush=True)
    if valid:
        with open('exoo35_graphs.json', 'w') as f:
            out = []
            for i in valid:
                adj = parse_g6(lines[i])
                out.append({'adjacency': [''.join('1' if (m >> j) & 1 else '0'
                                                 for j in range(len(adj)))
                                          for m in adj]})
            json.dump(out, f)
        print(f'wrote exoo35_graphs.json with {len(valid)} graphs', flush=True)


if __name__ == '__main__':
    main()
