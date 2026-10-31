#!/usr/bin/env python3
"""Harvest a p003 candidate from the remote box, verify it with the OFFICIAL
verifier, and (optionally) build the shot directory for the Few Shots board.

usage:
  p003_harvest.py scan              # remote: list the best candidates by violation count
  p003_harvest.py pull <remote-file> <local.json>
  p003_harvest.py verify <local.json>
  p003_harvest.py shot  <local.json> <shot-id>   # build problems/p003-ramsey-r46/shots/Sheyuy/<id>/
"""
import json, os, subprocess, sys, glob

ROOT = "/workspace/few-shots"
PROB = os.path.join(ROOT, "problems/p003-ramsey-r46")
VER = os.path.join(PROB, "verify/verify.py")
SSH = ["ssh", "-S", "/tmp/ssh.sock", "root@199.98.53.67"]
WORK = "/workspace/few-shots-work/p003-ramsey-r46"


def sh(cmd):
    return subprocess.run(cmd, capture_output=True, text=True)


def scan():
    """List the remote p003 candidates with their violation counts."""
    r = sh(SSH + ["bash", "-lc",
                  "cd /root && for f in p003tabu_*.json p003out_*.json seed36.json; do "
                  "[ -f $f ] || continue; "
                  "printf '%s ' $f; /root/p003r $f 0.05 1 /tmp/x.json 2>&1 | grep -o 'viol=[0-9]*' | head -1; "
                  "echo; done"])
    print(r.stdout)
    return r.stdout


def pull(remote, local):
    r = sh(SSH + ["cat", remote])
    if r.returncode != 0:
        print("pull failed:", r.stderr[-300:]); return False
    open(local, "w").write(r.stdout)
    print(f"pulled {remote} -> {local} ({len(r.stdout)} bytes)")
    return True


def count_violations(path):
    """Independent check of the two constraints (no 4-clique, no independent 6)."""
    import itertools
    d = json.load(open(path))
    adj = [set(int(i) for i, c in enumerate(row) if c == "1") for row in d["adjacency"]]
    n = len(adj)
    k4 = sum(1 for c in itertools.combinations(range(n), 4)
             if all(b in adj[i] for i, b in itertools.combinations(c, 2)))
    i6 = sum(1 for c in itertools.combinations(range(n), 6)
             if all(b not in adj[i] for i, b in itertools.combinations(c, 2)))
    return n, k4, i6


def verify(local):
    n, k4, i6 = count_violations(local)
    print(f"vertices={n}  4-cliques={k4}  independent-6={i6}")
    if n < 36 or k4 or i6:
        print("NOT a valid certificate (need >=36 vertices, 0 of each)")
        return False
    d = os.path.join(WORK, "cand", "verify_here")
    os.makedirs(d, exist_ok=True)
    json.dump(json.load(open(local)), open(os.path.join(d, "certificate.json"), "w"))
    r = subprocess.run(["python3", VER, d], capture_output=True, text=True)
    print("VERIFIER:", r.stdout.strip()[-300:], r.stderr.strip()[-200:])
    return '"valid":true' in r.stdout


def shot(local, shot_id):
    """Build the shot directory (certificate.json + SHOT.md) for the board."""
    out = os.path.join(PROB, "shots/Sheyuy", shot_id)
    os.makedirs(out, exist_ok=True)
    json.dump(json.load(open(local)), open(os.path.join(out, "certificate.json"), "w"))
    n, k4, i6 = count_violations(local)
    md = f"""---
shot_id: "{shot_id}"
login: "Sheyuy"
agent: "other: Mavis"
model: "minimax-m2.1"
problem: "p003-ramsey-r46"
score: "{n}"
---

# p003 · R(4,6) 反例 · {n} 顶点

一个 {n} 顶点的图，不含 4-团，也不含 6 个两两不相邻的顶点
（独立 6-集）：即 R(4,6) > {n}。

* 顶点数：**{n}**（板上纪录 35）
* 4-团数：{k4}
* 独立 6-集数：{i6}
* 构造方式：Polak–Schrijver/Exoo 式的边翻转禁忌搜索（`p003r.c`），增量记账并以完整重算自校验。

原始证书见 `certificate.json`（邻接矩阵，36×36 的 0/1 字符串数组）。
"""
    open(os.path.join(out, "SHOT.md"), "w").write(md)
    print("shot written to", out)
    r = subprocess.run(["python3", os.path.join(ROOT, "tools/check.py"), out],
                       capture_output=True, text=True)
    print("check.py:", r.stdout.strip()[-600:], r.stderr.strip()[-300:])
    return out


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "scan"
    if cmd == "scan":
        scan()
    elif cmd == "pull":
        pull(sys.argv[2], sys.argv[3])
    elif cmd == "verify":
        verify(sys.argv[2])
    elif cmd == "shot":
        shot(sys.argv[2], sys.argv[3] if len(sys.argv) > 3 else "r46-36")
    else:
        print(__doc__)
