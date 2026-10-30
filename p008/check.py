#!/usr/bin/env python3
"""p008 fast circuit checker / simplifier.

Usage: python3 check.py <certificate.json> [--walk SECONDS]

Verifies an AES S-box circuit over {AND, XOR, XNOR} with 256-bit truth tables,
then reports every generic simplification opportunity we know of:
  * dead gates / duplicate definitions / identical wires
  * ODC single-wire resubstitution (delete one gate)
  * 2->1 rewrite (g1 feeds only g2, g2 recomputable from two earlier wires)
  * forward substitution (a later wire equals an earlier one on its care set)
  * 3->1 rewrite (both operands are gates feeding only g2)
With --walk N it also runs the randomized resubstitution walk for N seconds
looking for a wire that becomes dead (a free gate).
"""
import json, re, sys, random, time
from pathlib import Path

ALL = (1 << 256) - 1
INPUTS = ["x%d" % i for i in range(8)]
VERIFY = Path(__file__).resolve().parents[2] / "few-shots" / "problems" / "p008-aes-sbox-circuit" / "verify" / "verify.py"


def sbox_table():
    txt = VERIFY.read_text()
    m = re.search(r'SBOX_HEX = """\\\n(.*?)"""', txt, re.S)
    return bytes.fromhex("".join(m.group(1).split()))


def ev(op, a, b):
    if op == "AND":
        return a & b
    if op == "XOR":
        return a ^ b
    return ALL ^ (a ^ b)


def load(path):
    d = json.load(open(path))
    return [dict(op=g["op"], a=g["a"], b=g["b"], out=g["out"]) for g in d["gates"]], list(d["outputs"])


def evaluate(gates):
    val = {w: 0 for w in INPUTS}
    for k in range(256):
        for i in range(8):
            if (k >> i) & 1:
                val["x%d" % i] |= 1 << k
    for g in gates:
        val[g["out"]] = ev(g["op"], val[g["a"]], val[g["b"]])
    return val


def observability(gates, outs, val):
    od = {w: 0 for w in val}
    for w in outs:
        od[w] = ALL
    for g in reversed(gates):
        w, op = g["out"], g["op"]
        if op == "AND":
            sa, sb = val[g["b"]], val[g["a"]]
        else:
            sa = sb = ALL
        od[g["a"]] |= od[w] & sa
        od[g["b"]] |= od[w] & sb
    return od


def fanout(gates, val):
    f = {w: 0 for w in val}
    for g in gates:
        f[g["a"]] += 1
        f[g["b"]] += 1
    return f


def outputs_ok(val, outs, S):
    for i, w in enumerate(outs):
        exp = 0
        for k in range(256):
            if (S[k] >> i) & 1:
                exp |= 1 << k
        if val[w] != exp:
            return False
    return True


def main():
    path = sys.argv[1]
    walk = 0.0
    if "--walk" in sys.argv:
        walk = float(sys.argv[sys.argv.index("--walk") + 1])
    S = sbox_table()
    gates, outs = load(path)
    outset = set(outs)
    val = evaluate(gates)
    ok = outputs_ok(val, outs, S)
    print("gates=%d valid=%s ops=%s" % (len(gates), ok, {}))
    from collections import Counter
    print("  ops:", dict(Counter(g["op"] for g in gates)))
    if not ok:
        return 1
    f = fanout(gates, val)
    dead = [g["out"] for g in gates if f.get(g["out"], 0) == 0 and g["out"] not in outset]
    print("  dead gates:", dead)
    seen, dup = {}, []
    for i, g in enumerate(gates):
        key = (g["op"],) + tuple(sorted((g["a"], g["b"])))
        if key in seen:
            dup.append((i, seen[key]))
        else:
            seen[key] = i
    print("  duplicate definitions:", dup)
    byval = {}
    for g in gates:
        byval.setdefault(val[g["out"]], []).append(g["out"])
    print("  identical-wire groups:", [v for v in byval.values() if len(v) > 1])

    od = observability(gates, outs, val)
    pos = {g["out"]: i for i, g in enumerate(gates)}

    # 1) ODC single-wire
    hits = []
    for i, g in enumerate(gates):
        w, o = g["out"], od[g["out"]]
        if o == 0:
            continue
        for u in INPUTS + [gg["out"] for gg in gates[:i]]:
            if u != w and ((val[w] ^ val[u]) & o) == 0:
                hits.append((i, w, u))
                break
    print("ODC single-wire deletions:", hits)

    # 2) 2->1 (one operand is a fanout-1 gate output)
    hits = []
    for i2, g2 in enumerate(gates):
        w2, o = g2["out"], od[g2["out"]]
        if o == 0:
            continue
        for which in ("a", "b"):
            w1 = g2[which]
            if w1 not in pos or f[w1] != 1 or w1 in outset:
                continue
            i1 = pos[w1]
            avail = INPUTS + [gg["out"] for gg in gates[:i1]]
            found = None
            for x in range(len(avail)):
                for y in range(x + 1, len(avail)):
                    u, v = avail[x], avail[y]
                    if u == w2 or v == w2:
                        continue
                    for op in ("AND", "XOR", "XNOR"):
                        if ((ev(op, val[u], val[v]) ^ val[w2]) & o) == 0:
                            found = (u, op, v)
                            break
                    if found:
                        break
                if found:
                    break
            if found:
                hits.append((i1, w1, i2, w2, found))
    print("2->1 rewrites:", hits)

    # 3) forward substitution
    def depends(j, w):
        stack, seen2 = [gates[j]["a"], gates[j]["b"]], set()
        while stack:
            x = stack.pop()
            if x == w:
                return True
            if x in seen2:
                continue
            seen2.add(x)
            if x in pos:
                stack += [gates[pos[x]]["a"], gates[pos[x]]["b"]]
        return False

    hits = []
    for i, g in enumerate(gates):
        w, o = g["out"], od[g["out"]]
        if o == 0:
            continue
        for j in range(i + 1, len(gates)):
            u = gates[j]["out"]
            if ((val[w] ^ val[u]) & o) == 0 and not depends(j, w):
                hits.append((i, w, j, u))
                break
    print("forward substitutions:", hits)

    # 4) 3->1
    def deletable(w):
        return w in pos and f[w] == 1 and w not in outset

    hits = []
    for i2, g2 in enumerate(gates):
        w2, o = g2["out"], od[g2["out"]]
        if o == 0:
            continue
        da, db = deletable(g2["a"]), deletable(g2["b"])
        if not (da or db):
            continue
        earliest = min(pos[x] for x, k in ((g2["a"], da), (g2["b"], db)) if k)
        avail = INPUTS + [gg["out"] for gg in gates[:earliest]]
        found = None
        for x in range(len(avail)):
            for y in range(x + 1, len(avail)):
                u, v = avail[x], avail[y]
                if u == w2 or v == w2:
                    continue
                for op in ("AND", "XOR", "XNOR"):
                    if ((ev(op, val[u], val[v]) ^ val[w2]) & o) == 0:
                        found = (u, op, v)
                        break
                if found:
                    break
            if found:
                break
        if found:
            hits.append((i2, w2, found))
    print("3->1 rewrites:", hits)

    if walk > 0:
        print("randomized walk for %.0fs ..." % walk)
        random.seed(12345)
        t0, it, applied = time.time(), 0, 0
        while time.time() - t0 < walk:
            it += 1
            i = random.randrange(len(gates))
            g = gates[i]
            w, o = g["out"], od[g["out"]]
            if o == 0:
                continue
            avail = INPUTS + [gg["out"] for gg in gates[:i]]
            if len(avail) < 2:
                continue
            u, v = random.choice(avail), random.choice(avail)
            if u == v or u == w or v == w:
                continue
            op = random.choice(("AND", "XOR", "XNOR"))
            if op == g["op"] and {u, v} == {g["a"], g["b"]}:
                continue
            if ((ev(op, val[u], val[v]) ^ val[w]) & o) != 0:
                continue
            old = (g["op"], g["a"], g["b"])
            g["op"], g["a"], g["b"] = op, u, v
            nval = evaluate(gates)
            if not outputs_ok(nval, outs, S):
                g["op"], g["a"], g["b"] = old
                continue
            val = nval
            od = observability(gates, outs, val)
            applied += 1
            f = fanout(gates, val)
            dead = [k for k, gg in enumerate(gates) if f.get(gg["out"], 0) == 0 and gg["out"] not in outset]
            if dead:
                print("DEAD GATES:", [gates[k]["out"] for k in dead], "at iter", it)
                for k in sorted(dead, reverse=True):
                    del gates[k]
                val = evaluate(gates)
                if outputs_ok(val, outs, S):
                    json.dump({"gates": gates, "outputs": outs}, open("better.json", "w"))
                    print("*** saved better.json with", len(gates), "gates ***")
                    return 0
                print("!! outputs broke after deletion")
        print("iterations=%d applied=%d gates=%d no dead gate" % (it, applied, len(gates)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
