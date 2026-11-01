#!/usr/bin/env python3
"""dispatch_sat.py -- fire and monitor the r46-sat GitHub Actions workflow.

Remote compute for the 36-vertex (4,6) SAT instances: the local sandbox has
only 1.3 CPU cores and reboots every couple of hours, which kills long SAT
runs.  A hosted runner gives an uninterrupted multi-hour job.

Usage:
  python3 dispatch_sat.py fire 11 12 13 14 15 16 17 --minutes 330
  python3 dispatch_sat.py fire plain --mode 5 --minutes 330
  python3 dispatch_sat.py list
  python3 dispatch_sat.py fetch <run_id>        # download artifacts
"""
import json
import os
import sys
import time
import urllib.error
import urllib.request

def _load_pat():
    """Read the token from the environment (FS_PAT) or from ~/.fs_pat.
    It is deliberately NOT hardcoded: this file lives in a public repository."""
    v = os.environ.get("FS_PAT")
    if v:
        return v.strip()
    for path in (os.path.expanduser("~/.fs_pat"), "/workspace/.fs_pat"):
        if os.path.exists(path):
            return open(path).read().strip()
    raise SystemExit("set FS_PAT or write the token to ~/.fs_pat")


PAT = _load_pat()
API = "https://api.github.com/repos/Sheyuy/few-shots-notes"
WF = "r46-sat.yml"
WF_LABS = "labs100.yml"


def req(method, path, data=None, raw=False, timeout=40):
    body = json.dumps(data).encode() if data is not None else None
    r = urllib.request.Request(API + path, data=body, method=method)
    r.add_header("Authorization", f"Bearer {PAT}")
    r.add_header("Accept", "application/vnd.github+json")
    r.add_header("X-GitHub-Api-Version", "2022-11-28")
    try:
        with urllib.request.urlopen(r, timeout=timeout) as resp:
            content = resp.read()
            return resp.status, (content if raw else json.loads(content.decode() or "{}"))
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode()[:300]
    except Exception as e:
        return 0, str(e)[:200]


def fire(d, mode, minutes, tag=""):
    st, out = req("POST", f"/actions/workflows/{WF}/dispatches",
                  {"ref": "main",
                   "inputs": {"d": str(d), "mode": str(mode), "minutes": str(minutes),
                              "tag": tag or f"{mode}_{d}"}})
    print(f"  dispatch d={d} mode={mode} minutes={minutes}: HTTP {st} {out if st not in (204,) else 'ok'}")
    return st == 204


def fire_labs(minutes, procs, seed, mode="ils", pop=32):
    st, out = req("POST", f"/actions/workflows/{WF_LABS}/dispatches",
                  {"ref": "main",
                   "inputs": {"minutes": str(minutes), "procs": str(procs),
                              "seed": str(seed), "mode": mode, "pop": str(pop),
                              "chunk": "200000"}})
    print(f"  dispatch labs100 seed={seed} procs={procs} mode={mode} minutes={minutes}: HTTP {st}")
    return st == 204


def list_labs(limit=20):
    st, d = req("GET", f"/actions/workflows/{WF_LABS}/runs?per_page={limit}")
    if st != 200:
        print("labs list failed", st, d)
        return []
    for r in d.get("workflow_runs", []):
        print(f"  labs run {r['id']}  {r['status']:<12} {str(r['conclusion']):<10} {r['created_at']}")
    return d.get("workflow_runs", [])


def list_runs(limit=20):
    st, d = req("GET", f"/actions/workflows/{WF}/runs?per_page={limit}")
    if st != 200:
        print("list failed", st, d)
        return []
    rows = []
    for r in d.get("workflow_runs", []):
        print(f"  run {r['id']}  {r['status']:<12} {str(r['conclusion']):<10} "
              f"{r['created_at']}  {r['html_url']}")
        rows.append(r)
    return rows


class NoRedirect(urllib.request.HTTPRedirectHandler):
    """The artifact download redirects to blob storage, which rejects the
    Authorization header -> follow the redirect by hand and drop the header."""

    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None


def fetch(run_id, outdir="/workspace/few-shots-work/p003-ramsey-r46/ci_out"):
    st, d = req("GET", f"/actions/runs/{run_id}/artifacts")
    if st != 200:
        print("artifacts failed", st, d)
        return
    os.makedirs(outdir, exist_ok=True)
    opener = urllib.request.build_opener(NoRedirect)
    for a in d.get("artifacts", []):
        aid = a["id"]
        name = a["name"]
        url = f"{API}/actions/artifacts/{aid}/zip"
        r = urllib.request.Request(url)
        r.add_header("Authorization", f"Bearer {PAT}")
        r.add_header("Accept", "application/vnd.github+json")
        try:
            with opener.open(r, timeout=180) as resp:
                blob = resp.read()
        except urllib.error.HTTPError as e:
            if e.code not in (301, 302, 303, 307, 308):
                print("  download failed", aid, e.code, e.read()[:120])
                continue
            loc = e.headers.get("Location")
            if not loc:
                print("  no redirect location for", aid)
                continue
            with urllib.request.urlopen(urllib.request.Request(loc), timeout=300) as resp:
                blob = resp.read()
        path = os.path.join(outdir, f"{name}.zip")
        open(path, "wb").write(blob)
        print(f"  saved {path} ({len(blob)} bytes)")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    cmd = sys.argv[1]
    if cmd == "fire":
        mode = 4
        minutes = 330
        args = sys.argv[2:]
        if "--mode" in args:
            i = args.index("--mode"); mode = int(args[i + 1]); del args[i:i + 2]
        if "--minutes" in args:
            i = args.index("--minutes"); minutes = int(args[i + 1]); del args[i:i + 2]
        for d in args:
            dd = d if d == "plain" else int(d)
            fire(dd, mode, minutes)
            time.sleep(2)
    elif cmd == "list":
        list_runs()
        list_labs()
    elif cmd == "firelabs":
        args = sys.argv[2:]
        minutes, procs, seed, mode = 330, 2, 1, "ils"
        if "--minutes" in args:
            i = args.index("--minutes"); minutes = int(args[i+1]); del args[i:i+2]
        if "--procs" in args:
            i = args.index("--procs"); procs = int(args[i+1]); del args[i:i+2]
        if "--seed" in args:
            i = args.index("--seed"); seed = int(args[i+1]); del args[i:i+2]
        if "--mode" in args:
            i = args.index("--mode"); mode = args[i+1]; del args[i:i+2]
        fire_labs(minutes, procs, seed, mode)
    elif cmd == "fetchlabs":
        for rid in sys.argv[2:]:
            print(f"fetch labs run {rid}")
            fetch(rid, outdir="/workspace/few-shots-work/p006-labs-100/ci_out")
    elif cmd == "fetch":
        for rid in sys.argv[2:]:
            print(f"fetch run {rid}")
            fetch(rid)
    else:
        print(__doc__)
    return 0


if __name__ == "__main__":
    sys.exit(main())
