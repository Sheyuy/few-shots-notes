#!/usr/bin/env python3
"""Linear-block analysis for the p008 AES S-box circuit.

Key facts printed:
  * stage 0 (the initial linear map feeding the first 9 AND gates): number of
    distinct target forms  ->  a lower bound on its XOR gate count.
  * stage 33 (the final linear map producing the 8 outputs): greedy SLP size vs
    the 24 gates used by the current circuit.

Usage: python3 slp.py [certificate.json]
"""
import json
import sys

CONST = 1 << 62


def sigs(gates):
    sig = {"x%d" % i: 1 << i for i in range(8)}
    nand = 0
    for g in gates:
        if g["op"] == "AND":
            sig[g["out"]] = 1 << (8 + nand)
            nand += 1
        else:
            s = sig[g["a"]] ^ sig[g["b"]]
            if g["op"] == "XNOR":
                s ^= CONST
            sig[g["out"]] = s
    return sig, nand


def greedy(inputs, targets, cap=80):
    """Return (gates, unresolved).  Polarity-aware: XNOR costs the same as XOR."""
    avail = set(inputs)
    for m in list(avail):
        avail.add(m ^ CONST)
    tg = list(dict.fromkeys(targets))
    resolved = set(t for t in tg if t in avail)
    gates = 0
    while len(resolved) < len(tg) and gates < cap:
        # 1) free or 1-gate resolutions
        progress = True
        while progress:
            progress = False
            for t in tg:
                if t in resolved:
                    continue
                if t in avail:
                    resolved.add(t)
                    progress = True
                    continue
                found = False
                for u in avail:
                    if (t ^ u) in avail:
                        found = True
                        break
                if found:
                    avail.add(t)
                    avail.add(t ^ CONST)
                    resolved.add(t)
                    gates += 1
                    progress = True
        if len(resolved) == len(tg):
            break
        # 2) add the intermediate that most reduces the remaining targets
        L = list(avail)
        best, bestsc = None, -1
        for i in range(len(L)):
            for j in range(i + 1, len(L)):
                x = L[i] ^ L[j]
                if x in avail or (x ^ CONST) in avail:
                    continue
                sc = 0
                for t in tg:
                    if t in resolved:
                        continue
                    if (t ^ x).bit_count() < t.bit_count():
                        sc += 1
                    if t == x:
                        sc += 5
                if sc > bestsc:
                    bestsc, best = sc, x
        if best is None or bestsc <= 0:
            break
        avail.add(best)
        avail.add(best ^ CONST)
        gates += 1
    return gates, [t for t in tg if t not in resolved]


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else (
        "few-shots/problems/p008-aes-sbox-circuit/shots/huangxin-design/"
        "aes102-field-plane-offsets/certificate.json")
    gates = json.load(open(path))["gates"]
    outs = json.load(open(path))["outputs"]
    sig, nand = sigs(gates)

    # first 9 AND gates: operands are computed purely from x (stage-0 linear block)
    t0 = []
    cnt = 0
    for g in gates:
        if g["op"] == "AND":
            t0 += [sig[g["a"]], sig[g["b"]]]
            cnt += 1
            if cnt == 9:
                break
    t0 = list(dict.fromkeys(t0))
    print("stage-0 block: %d distinct target forms -> lower bound %d gates" % (len(t0), len(t0)))
    print("  (current circuit uses 18)")

    t33 = [sig[w] for w in outs]
    print("\nstage-33 block: %d targets, weights %s" % (len(t33), [t.bit_count() for t in t33]))
    in33 = [1 << i for i in range(8)] + [1 << (8 + k) for k in range(nand)]
    g33, rem = greedy(in33, t33, cap=40)
    print("  greedy SLP: %d gates, unresolved %d   (current circuit uses 24)" % (g33, len(rem)))

    # middle: all AND operands after the 9th, computed with x + first 9 AND outputs
    tm = []
    cnt = 0
    for g in gates:
        if g["op"] == "AND":
            cnt += 1
            if cnt > 9:
                tm += [sig[g["a"]], sig[g["b"]]]
    tm = list(dict.fromkeys(tm))
    inm = [1 << i for i in range(8)] + [1 << (8 + k) for k in range(9)]
    gm, remm = greedy(inm, tm, cap=60)
    print("\nmiddle block: %d distinct target forms" % len(tm))
    print("  greedy SLP: %d gates, unresolved %d   (current circuit uses 15+12=27)" % (gm, len(remm)))


if __name__ == "__main__":
    main()
