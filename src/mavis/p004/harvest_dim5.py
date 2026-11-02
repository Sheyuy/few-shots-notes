#!/usr/bin/env python3
"""Turn a dim-5 independent set (one 5-digit word per line) into a dim-10
certificate via the strong-product (x) construction, then run the OFFICIAL
verifier.  |A x A| = |A|^2 points in C_7^10.
Usage: harvest_dim5.py <dim5file> <shot-id>"""
import json, os, subprocess, sys

src = sys.argv[1]
shot_id = sys.argv[2] if len(sys.argv) > 2 else "dim5-square"
A = [l.strip() for l in open(src) if l.strip()]
assert A and all(len(w) == 5 and set(w) <= set("0123456") for w in A), "bad dim5 file"
words = [a + b for a in A for b in A]
print(f"dim5 points = {len(A)}  ->  dim10 words = {len(words)}")

# local sanity: independence inside C_7^5
def adj(u, v):
    return all((int(x) - int(y)) % 7 in (0, 1, 6) for x, y in zip(u, v))
S = set(A)
for i, u in enumerate(A):
    for v in A[i+1:]:
        if adj(u, v):
            print("FATAL: dim5 set is not independent", u, v); sys.exit(2)
print("dim5 independence OK")

root = "/workspace/few-shots"
# default: write the certificate into the WORK area; the repo is only touched
# for a real record-breaking shot (and even then only by mkshot.sh).
dest_root = os.environ.get("SHOT_DEST_ROOT", "/workspace/few-shots-work/p004-shannon-capacity-c7/certs")
out = os.path.join(dest_root, shot_id)
os.makedirs(out, exist_ok=True)
cert = os.path.join(out, "certificate.json")
with open(cert, "w") as f:
    json.dump({"points": words}, f)
print("certificate:", cert, os.path.getsize(cert), "bytes")

r = subprocess.run(["python3", os.path.join(root, "problems/p004-shannon-capacity-c7/verify/verify.py"), out],
                   capture_output=True, text=True)
print("VERIFIER:", r.stdout.strip()[-400:], r.stderr.strip()[-200:])
