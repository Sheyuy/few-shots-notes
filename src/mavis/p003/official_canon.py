#!/usr/bin/env python3
"""Official-verify every graph in a .txt file and count distinct isomorphism
classes (via the verifier's canonical_hash).

The official verifier canonicalises a valid graph up to isomorphism, so its
canonical_hash is an exact isomorphism-class identifier.  That lets us measure
how much *genuinely new* structure a generator really produced -- labelled
dedup is not enough.

Usage: python3 official_canon.py <graphs.txt> [out.json]
"""
import json
import os
import subprocess
import sys
import tempfile
from collections import defaultdict

VERIFY = "/workspace/few-shots/problems/p003-ramsey-r46/verify/verify.py"


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


def to_adj(G):
    n = len(G)
    rows = []
    for i in range(n):
        rows.append(''.join('1' if (G[i] >> j) & 1 else '0' for j in range(n)))
    return rows


def main():
    gfile = sys.argv[1]
    outjson = sys.argv[2] if len(sys.argv) > 2 else None
    graphs = read_graphs(gfile)
    print(f'{gfile}: {len(graphs)} graphs', flush=True)
    d = tempfile.mkdtemp(prefix='canon')
    hashes = defaultdict(list)
    invalid = []
    for gi, G in enumerate(graphs):
        with open(os.path.join(d, 'certificate.json'), 'w') as f:
            json.dump({'adjacency': to_adj(G)}, f)
        r = subprocess.run([sys.executable, VERIFY, d], capture_output=True, text=True,
                           timeout=600)
        try:
            res = json.loads(r.stdout.strip().splitlines()[-1])
        except Exception:
            invalid.append((gi, 'PARSE', r.stdout[-200:], r.stderr[-200:]))
            continue
        if not res.get('valid'):
            invalid.append((gi, 'INVALID', r.stdout[:200]))
            continue
        hashes[res['canonical_hash']].append(gi)
        if (gi + 1) % 100 == 0:
            print(f'  ... {gi+1}/{len(graphs)}  distinct classes so far = {len(hashes)}',
                  flush=True)
    print(f'--- valid: {sum(len(v) for v in hashes.values())}/{len(graphs)}')
    print(f'--- distinct isomorphism classes: {len(hashes)}')
    for h, idxs in sorted(hashes.items(), key=lambda kv: -len(kv[1]))[:20]:
        print(f'   {h[:16]}  x{len(idxs)}  e.g. g{idxs[0]}')
    if invalid:
        print(f'--- invalid/error: {len(invalid)}')
        for x in invalid[:5]:
            print('   ', x)
    if outjson:
        json.dump({h: v for h, v in hashes.items()}, open(outjson, 'w'))
    return 0


if __name__ == '__main__':
    sys.exit(main())
