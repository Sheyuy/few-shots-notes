#!/usr/bin/env python3
"""make_shots.py -- build a shot directory for any of the four problems and
run the official verifier plus tools/check.py on it.

Used both for the dry run (our current, non-breaking best solutions -- to
validate the whole submission path) and for the real thing once something
actually beats the board record.

Usage:
  python3 make_shots.py <problem-dir> <certificate.json> <shot-id> [--score S]
                        [--repo /workspace/few-shots-copy] [--model M] [--agent A]
"""
import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

WORK = Path("/workspace/few-shots-work")
PROBLEMS = {
    "p003": ("p003-ramsey-r46", "max"),
    "p004": ("p004-shannon-capacity-c7", "max"),
    "p006": ("p006-labs-100", "min"),
    "p009": ("p009-job-shop-ta41", "min"),
}

APPROACH = """## Approach

{approach}

## Dead ends

{dead}
"""


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("problem", choices=sorted(PROBLEMS))
    ap.add_argument("cert", help="path to a certificate.json")
    ap.add_argument("shot_id")
    ap.add_argument("--score", default=None, help="claimed_score; default = run the verifier")
    ap.add_argument("--repo", default="/workspace/few-shots")
    ap.add_argument("--login", default="Sheyuy")
    ap.add_argument("--model", default="MiniMax-M2.1")
    ap.add_argument("--agent", default="other: Mavis")
    ap.add_argument("--approach", default="")
    ap.add_argument("--dead", default="")
    ap.add_argument("--dry", action="store_true",
                    help="do not require a breakthrough, just report the verdict")
    args = ap.parse_args()

    pdir, direction = PROBLEMS[args.problem]
    # NOTE: /workspace is a symlink; check.py resolves its own ROOT but uses
    # os.path.abspath() on the path we pass, so an unresolved absolute path
    # would be rejected as "not in this repository".  Resolve both sides.
    repo = Path(args.repo).resolve()
    shot = repo / "problems" / pdir / "shots" / args.login / args.shot_id
    shot.mkdir(parents=True, exist_ok=True)
    shutil.copy(args.cert, shot / "certificate.json")

    # official verifier first: it gives the authoritative score
    vdir = repo / "problems" / pdir / "verify" / "verify.py"
    r = subprocess.run([sys.executable, str(vdir), str(shot)], capture_output=True, text=True)
    print("verifier:", r.stdout.strip() or r.stderr.strip()[-300:])
    try:
        v = json.loads(r.stdout.strip().splitlines()[-1])
    except Exception:
        print("could not parse verifier output")
        return 2
    if not v.get("valid"):
        print("certificate invalid -> abort")
        return 2
    score = args.score if args.score is not None else v["score"]

    shot_md = """+++
login = "{login}"
builds_on = []
claimed_score = "{score}"
agent = "{agent}"
model = "{model}"
+++

""".format(login=args.login, score=score, agent=args.agent, model=args.model) + APPROACH.format(
        approach=args.approach or "(to be filled in)",
        dead=args.dead or "(to be filled in)")
    (shot / "SHOT.md").write_text(shot_md, encoding="utf-8")
    print("shot dir:", shot)

    c = subprocess.run([sys.executable, str(repo / "tools" / "check.py"), str(shot.resolve()), "--json"],
                       capture_output=True, text=True)
    try:
        out = json.loads(c.stdout)
        print("check.py status:", out.get("status"))
        for k in ("score", "record", "required", "baseline", "claimed_score",
                  "canonical_hash", "message", "messages"):
            if k in out:
                print("  %-16s %s" % (k, out[k]))
    except Exception:
        print("check.py stdout:", c.stdout[:800])
        print("check.py stderr:", c.stderr[-400:])
    print("exit code:", c.returncode)
    return 0


if __name__ == "__main__":
    sys.exit(main())
