#!/usr/bin/env python3
"""Generate many *diverse* valid 35-vertex (4,6)-graphs and test each one for
extendability to 36 vertices.

Why this and not the sibling operation
--------------------------------------
The sibling operation ((G\\u) + v) applied to the 37 known graphs only produced
~13 isomorphism classes out of 1293 labelled graphs -- far too narrow.  Local
search on 35 vertices, however, converges to a valid 35-vertex graph in about
one second from a perturbed seed, and the valid 35-vertex graphs form a huge
space.

For each fresh valid 35-vertex graph B we then run the *exact* extension SAT
(35 variables, ~1200 clauses, milliseconds): is there a neighbourhood S such
that B + v is a valid 36-vertex (4,6)-graph?  A YES is R(4,6) >= 37.

Usage: python3 gen35.py <outdir> [seconds] [workers]
"""
import json
import os
import random
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
P003R = os.path.join(HERE, 'p003r')
EXTSAT = os.path.join(HERE, 'extsat')
MINISAT = os.path.join(HERE, 'minisat_bin')


# --------------------------------------------------------------- helpers
def read_graphs(path):
    out = []
    with open(path) as f:
        for line in f:
            p = line.split()
            if not p:
                continue
            n = int(p[0])
            out.append([int(x, 16) for x in p[1:1 + n]])
    return out


def rows_to_masks(rows):
    n = len(rows)
    out = []
    for i in range(n):
        m = 0
        for j in range(n):
            if rows[i][j] == '1':
                m |= 1 << j
        out.append(m)
    return out


def score(masks):
    """K4 / I6 count of a bitmask adjacency (independent of the solvers)."""
    n = len(masks)
    full = (1 << n) - 1
    k4 = 0
    for j in range(n):
        gt = ~((1 << (j + 1)) - 1) & full
        for i in range(j):
            if (masks[i] >> j) & 1:
                c = masks[i] & masks[j] & gt
                while c:
                    lo = c & -c
                    v = lo.bit_length() - 1
                    c ^= lo
                    k4 += bin(c & masks[v]).count('1')
    nonadj = [full & ~masks[i] & ~(1 << i) for i in range(n)]

    def count(mask, size):
        if size == 0:
            return 1
        total = 0
        while mask:
            lo = mask & (-mask)
            v = lo.bit_length() - 1
            mask ^= lo
            total += count(mask & nonadj[v], size - 1)
        return total

    return k4, count(full, 6)


def wl_hash(masks, rounds=4):
    """Colour-refinement fingerprint -- cheap near-isomorphism test."""
    n = len(masks)
    colours = [0] * n
    for _ in range(rounds):
        sig = [(bin(masks[i]).count('1'), colours[i]) for i in range(n)]
        uniq = sorted(set(sig))
        rank = {s: r for r, s in enumerate(uniq)}
        colours = [rank[s] for s in sig]
    return tuple(sorted(colours))


def perturb(masks, rng, k):
    n = len(masks)
    out = list(masks)
    for _ in range(k):
        i, j = rng.randrange(n), rng.randrange(n)
        if i == j:
            continue
        if (out[i] >> j) & 1:
            out[i] &= ~(1 << j)
            out[j] &= ~(1 << i)
        else:
            out[i] |= 1 << j
            out[j] |= 1 << i
    return out


def write_masks(path, masks):
    with open(path, 'w') as f:
        f.write(str(len(masks)) + ' ' + ' '.join('%x' % m for m in masks) + '\n')


def to_json(masks):
    n = len(masks)
    return {'adjacency': [''.join('1' if (m >> j) & 1 else '0' for j in range(n))
                          for m in masks]}


# --------------------------------------------------------------- worker
def worker(wid, outdir, seconds, seedpool_file, state):
    rng = random.Random(1000 * wid + 7)
    pool = read_graphs(seedpool_file)
    seen = set()
    tmp_seed = os.path.join(outdir, f'w{wid}_seed.json')
    tmp_out = os.path.join(outdir, f'w{wid}_out.json')
    tmp_g = os.path.join(outdir, f'w{wid}_g.txt')
    wdir = os.path.join(outdir, f'w{wid}')
    os.makedirs(wdir, exist_ok=True)
    pool_file = os.path.join(outdir, 'pool35.txt')
    t0 = time.time()
    tried = 0
    good = 0
    hits = 0
    while time.time() - t0 < seconds:
        base = pool[rng.randrange(len(pool))]
        k = rng.randint(2, 7)
        cand = perturb(base, rng, k)
        json.dump(to_json(cand), open(tmp_seed, 'w'))
        r = subprocess.run([P003R, 'tabu', tmp_seed, '2', str(rng.randrange(10 ** 9)),
                            tmp_out], capture_output=True, text=True, timeout=120)
        tried += 1
        if not os.path.exists(tmp_out):
            continue
        try:
            rows = json.load(open(tmp_out))['adjacency']
        except Exception:
            continue
        if len(rows) != 35:
            continue
        masks = rows_to_masks(rows)
        k4, i6 = score(masks)
        if k4 or i6:
            continue
        good += 1
        h = wl_hash(masks)
        if h in seen:
            continue
        seen.add(h)
        pool.append(masks)          # feed new classes back into the pool
        with open(pool_file, 'a') as pf:
            pf.write(str(len(masks)) + ' ' + ' '.join('%x' % m for m in masks) + '\n')
        write_masks(tmp_g, masks)
        # exact extension test
        out = subprocess.run([EXTSAT, tmp_g, MINISAT, wdir],
                             capture_output=True, text=True, timeout=300)
        if 'valid 36-vertex' in out.stdout:
            hits += 1
            print(f'*** WORKER {wid} HIT: {out.stdout.strip()}', flush=True)
        if (good % 10) == 0 or tried % 25 == 0:
            print(f'w{wid}: tried={tried} valid={good} new_classes={len(seen)} '
                  f'hits={hits} {int(time.time()-t0)}s', flush=True)
    print(f'w{wid}: DONE tried={tried} valid={good} new_classes={len(seen)} '
          f'hits={hits}', flush=True)
    return tried, good, len(seen), hits


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else 'gen35_out'
    seconds = int(sys.argv[2]) if len(sys.argv) > 2 else 600
    nworkers = int(sys.argv[3]) if len(sys.argv) > 3 else 2
    os.makedirs(outdir, exist_ok=True)
    seedpool = os.path.join(HERE, 'g35.txt')
    if not os.path.exists(seedpool):
        print('missing g35.txt')
        return 1
    procs = []
    for w in range(nworkers):
        p = subprocess.Popen([sys.executable, os.path.abspath(__file__),
                              '--worker', str(w), outdir, str(seconds), seedpool])
        procs.append(p)
    for p in procs:
        p.wait()
    print('all workers done', flush=True)
    return 0


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == '--worker':
        wid = int(sys.argv[2])
        outdir = sys.argv[3]
        seconds = int(sys.argv[4])
        seedpool = sys.argv[5]
        worker(wid, outdir, seconds, seedpool, None)
        sys.exit(0)
    sys.exit(main())
