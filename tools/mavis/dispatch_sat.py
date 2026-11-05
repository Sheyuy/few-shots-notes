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
WORK = "/workspace/few-shots-work"
WF = "r46-sat.yml"
WF_LABS = "labs100.yml"
WF_TA41 = "ta41.yml"
WF_P004 = "p004-dim5.yml"
WF_NEAR = "p003-near.yml"


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


def fire(d, mode, minutes, tag="", solver="kissat"):
    st, out = req("POST", f"/actions/workflows/{WF}/dispatches",
                  {"ref": "main",
                   "inputs": {"d": str(d), "mode": str(mode), "minutes": str(minutes),
                              "tag": tag or f"{mode}_{d}", "solver": solver}})
    print(f"  dispatch d={d} mode={mode} minutes={minutes}: HTTP {st} {out if st not in (204,) else 'ok'}")
    return st == 204


def fire_labs(minutes, procs, seed, mode="ils", pop=32, warm=""):
    st, out = req("POST", f"/actions/workflows/{WF_LABS}/dispatches",
                  {"ref": "main",
                   "inputs": {"minutes": str(minutes), "procs": str(procs),
                              "seed": str(seed), "mode": mode, "pop": str(pop),
                              "chunk": "200000", "warm": warm}})
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


def fire_ta41(minutes, procs, seed, burst=200, bridge=2, nb=2):
    st, out = req("POST", f"/actions/workflows/{WF_TA41}/dispatches",
                  {"ref": "main",
                   "inputs": {"minutes": str(minutes), "procs": str(procs),
                              "seed": str(seed), "burst": str(burst),
                              "bridge": str(bridge), "nb": str(nb)}})
    print(f"  dispatch ta41 seed={seed} procs={procs} minutes={minutes}: HTTP {st}")
    return st == 204


def list_ta41(limit=20):
    st, d = req("GET", f"/actions/workflows/{WF_TA41}/runs?per_page={limit}")
    if st != 200:
        print("ta41 list failed", st, d)
        return []
    for r in d.get("workflow_runs", []):
        print(f"  ta41 run {r['id']}  {r['status']:<12} {str(r['conclusion']):<10} {r['created_at']}")
    return d.get("workflow_runs", [])


def fire_p004(kmin, kmax, procs=2):
    st, out = req("POST", f"/actions/workflows/{WF_P004}/dispatches",
                  {"ref": "main",
                   "inputs": {"kmin": str(kmin), "kmax": str(kmax), "procs": str(procs),
                              "rfile": "src/mavis/p004/R5.txt"}})
    print(f"  dispatch p004-dim5 k={kmin}..{kmax} procs={procs}: HTTP {st}")
    return st == 204


def list_p004(limit=20):
    st, d = req("GET", f"/actions/workflows/{WF_P004}/runs?per_page={limit}")
    if st != 200:
        print("p004 list failed", st, d)
        return []
    for r in d.get("workflow_runs", []):
        print(f"  p004 run {r['id']}  {r['status']:<12} {str(r['conclusion']):<10} {r['created_at']}")
    return d.get("workflow_runs", [])


def fire_near(ks, graphs, minutes=30, seed=7, mode="k", nfree=3):
    st, out = req("POST", f"/actions/workflows/{WF_NEAR}/dispatches",
                  {"ref": "main",
                   "inputs": {"ks": ks, "graphs": graphs, "minutes": str(minutes),
                              "seed": str(seed), "mode": mode, "nfree": str(nfree)}})
    print(f"  dispatch p003-near ks=[{ks}] graphs=[{graphs}]: HTTP {st}")
    return st == 204


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
        solver = "kissat"
        args = sys.argv[2:]
        if "--mode" in args:
            i = args.index("--mode"); mode = int(args[i + 1]); del args[i:i + 2]
        if "--minutes" in args:
            i = args.index("--minutes"); minutes = int(args[i + 1]); del args[i:i + 2]
        if "--solver" in args:
            i = args.index("--solver"); solver = args[i + 1]; del args[i:i+2]
        for d in args:
            dd = d if d == "plain" else int(d)
            fire(dd, mode, minutes, solver=solver)
            time.sleep(2)
    elif cmd == "list":
        list_runs()
        list_labs()
        list_ta41()
        list_p004()
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
        warm = ""
        if "--warm" in args:
            i = args.index("--warm"); warm = args[i+1]; del args[i:i+2]
        fire_labs(minutes, procs, seed, mode, warm=warm)
    elif cmd == "fireta41":
        args = sys.argv[2:]
        minutes, procs, seed, nb = 330, 2, 1, 2
        for flag, var in (("--minutes", "minutes"), ("--procs", "procs"), ("--seed", "seed")):
            if flag in args:
                i = args.index(flag); v = int(args[i+1]); del args[i:i+2]
                if var == "minutes": minutes = v
                elif var == "procs": procs = v
                else: seed = v
        if "--nb" in args:
            i = args.index("--nb"); nb = int(args[i+1]); del args[i:i+2]
        fire_ta41(minutes, procs, seed, nb=nb)
    elif cmd == "firenear":
        args = sys.argv[2:]
        ks = "0,4,8,16,32,64,128"; graphs = "0,1,2,3,4"; minutes = 30; seed = 7; mode = "k"
        if "--ks" in args:
            i = args.index("--ks"); ks = args[i+1]; del args[i:i+2]
        if "--graphs" in args:
            i = args.index("--graphs"); graphs = args[i+1]; del args[i:i+2]
        if "--minutes" in args:
            i = args.index("--minutes"); minutes = int(args[i+1]); del args[i:i+2]
        if "--seed" in args:
            i = args.index("--seed"); seed = int(args[i+1]); del args[i:i+2]
        if "--mode" in args:
            i = args.index("--mode"); mode = args[i+1]; del args[i:i+2]
        nfree = 3
        if "--nfree" in args:
            i = args.index("--nfree"); nfree = int(args[i+1]); del args[i:i+2]
        fire_near(ks, graphs, minutes, seed, mode, nfree)
    elif cmd == "fetchnear":
        for rid in sys.argv[2:]:
            print(f"fetch near run {rid}")
            fetch(rid, outdir="/workspace/few-shots-work/p003-ramsey-r46/ci_near")
    elif cmd == "firep004":
        args = sys.argv[2:]
        kmin, kmax, procs = 5, 16, 2
        if "--kmin" in args:
            i = args.index("--kmin"); kmin = int(args[i+1]); del args[i:i+2]
        if "--kmax" in args:
            i = args.index("--kmax"); kmax = int(args[i+1]); del args[i:i+2]
        if "--procs" in args:
            i = args.index("--procs"); procs = int(args[i+1]); del args[i:i+2]
        fire_p004(kmin, kmax, procs)
    elif cmd == "listp004":
        list_p004()
    elif cmd == "fetchp004":
        for rid in sys.argv[2:]:
            print(f"fetch p004 run {rid}")
            fetch(rid, outdir="/workspace/few-shots-work/p004-shannon-capacity-c7/ci_out")
    elif cmd == "listta41":
        list_ta41()
        list_p004()
    elif cmd == "fetchta41":
        for rid in sys.argv[2:]:
            print(f"fetch ta41 run {rid}")
            fetch(rid, outdir="/workspace/few-shots-work/p009-job-shop-ta41/ci_out")
    elif cmd == "fetchlabs":
        for rid in sys.argv[2:]:
            print(f"fetch labs run {rid}")
            fetch(rid, outdir="/workspace/few-shots-work/p006-labs-100/ci_out")
    elif cmd == "fetch":
        for rid in sys.argv[2:]:
            print(f"fetch run {rid}")
            # figure out which workflow this run belongs to, so the artifacts
            # land in the right directory
            try:
                st, info = req("GET", f"/actions/runs/{rid}")
                wf = info.get("path", "").split("/")[-1].replace(".yml", "")
            except Exception:
                wf = "unknown"
            outdir = {
                "r46-sat": f"{WORK}/p003-ramsey-r46/ci_out",
                "p003-near": f"{WORK}/p003-ramsey-r46/ci_near",
                "labs100": f"{WORK}/p006-labs-100/ci_out",
                "ta41": f"{WORK}/p009-job-shop-ta41/ci_out",
                "p009-cpsat": f"{WORK}/p009-job-shop-ta41/ci_cpsat",
                "p004-dim5": f"{WORK}/p004-shannon-capacity-c7/ci_out",
                "p004-dim5-cpsat": f"{WORK}/p004-shannon-capacity-c7/ci_cpsat",
                "p014-golomb": f"{WORK}/p014-golomb-ruler-29/ci_out",
                "p011-sortnet": f"{WORK}/p011-sorting-network-17/ci_out",
                "p003-pool": f"{WORK}/p003-ramsey-r46/ci_pool",
                "p003-enum": f"{WORK}/p003-ramsey-r46/ci_enum",
                "p003-enum-quick": f"{WORK}/p003-ramsey-r46/ci_enum",
            }.get(wf, f"{WORK}/harvest/{wf}")
            print("   ->", outdir)
            fetch(rid, outdir=outdir)
    else:
        print(__doc__)
    return 0


if __name__ == "__main__":
    sys.exit(main())
