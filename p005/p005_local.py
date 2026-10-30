#!/usr/bin/env python3
"""Cap-set local search for F_3^8 (p005). Baseline 512-cap is maximal, so we try
remove-r-points-then-greedily-regrow to try to exceed 512. Incremental forbidden
count keeps regrow fast. Runs until the time budget; prints the best size found."""
import json, random, time, sys

N = 8
SIZE = 3 ** N

def enc(t):
    v = 0
    for c in t:
        v = v * 3 + c
    return v

def third(a, b):
    va, vb = a, b
    c = 0
    mult = 1
    for _ in range(N):
        cc = (-(va % 3 + vb % 3)) % 3
        c += cc * mult
        mult *= 3
        va //= 3
        vb //= 3
    return c

base = json.load(open("../../few-shots/problems/p005-cap-set-8d/baseline/certificate.json"))["points"]
base_ints = [enc(tuple(p)) for p in base]
allpts = list(range(SIZE))

S = set(base_ints)
fc = [0] * SIZE  # forbidden count

def add(x):
    S.add(x)
    for y in S:
        if y == x:
            continue
        fc[third(x, y)] += 1

def remove(x):
    for y in S:
        if y == x:
            continue
        fc[third(x, y)] -= 1
    S.remove(x)

for x in base_ints:
    add(x)

def regrow():
    while True:
        addable = [p for p in allpts if p not in S and fc[p] == 0]
        if not addable:
            break
        add(random.choice(addable))

budget = float(sys.argv[1]) if len(sys.argv) > 1 else 120.0
seed = int(sys.argv[2]) if len(sys.argv) > 2 else 1
random.seed(seed)
best = set(S)
best_size = len(best)
t0 = time.time()
it = 0
while time.time() - t0 < budget:
    it += 1
    r = random.randint(1, 6)
    rem = random.sample(list(best), r)
    for x in rem:
        remove(x)
    regrow()
    if len(S) > best_size:
        best_size = len(S)
        best = set(S)
        print(f"iter {it}: new best size {best_size}", flush=True)
    # reset S to best for next kick
    S = set(best)
    fc = [0] * SIZE
    for x in best:
        add(x)

print(f"done iters={it} best_size={best_size}", flush=True)
if best_size > 512:
    pts = [list(dec(x)) for x in best] if False else None
    # decode
    out = []
    for x in best:
        t = [0] * N
        v = x
        for i in range(N - 1, -1, -1):
            t[i] = v % 3
            v //= 3
        out.append(t)
    json.dump({"points": out}, open("best_capset.json", "w"))
    print("WROTE best_capset.json with", best_size, "points")
