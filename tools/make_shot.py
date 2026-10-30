#!/usr/bin/env python3
"""Build a Few Shots shot directory and self-check it.

Usage (only when a search has beaten the record by >= epsilon):
    python3 make_shot.py <problem_id> <login> <shot_id> <candidate_file>

  problem_id : p006-labs-100  or  p009-job-shop-ta41  (directory under problems/)
  login      : your exact GitHub login (case-sensitive)
  shot_id    : lowercase letters/digits/hyphens, <=64 chars, not used before
  candidate_file:
      for p006 -> a 100-char '+''-' text file (the best sequence)
      for p009 -> best_orders.txt (20 lines of 30 job numbers)

It writes problems/<id>/shots/<login>/<shot_id>/{certificate.json,SHOT.md}
then runs tools/check.py and prints the verdict.
"""
import json, subprocess, sys, os
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent / "few-shots"
AGENT = "codebuddy"
MODEL = "other: WorkBuddy"

def main():
    if len(sys.argv) != 5:
        print(__doc__); sys.exit(2)
    pid, login, sid, cand = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
    pdir = REPO / "problems" / pid
    shot_dir = pdir / "shots" / login / sid
    shot_dir.mkdir(parents=True, exist_ok=True)

    if pid == "p006-labs-100":
        seq = open(cand).read().strip()
        assert set(seq) <= set("+-"), "p006 candidate must be '+'/'-' only"
        cert = {"sequence": seq}
        approach = ("Simulated annealing with incremental O(N) energy updates and iterated "
                    "local search (restart from the global best, perturbed, then re-annealed). "
                    "Built on the known best 578 sequence as the seed and escaped its basin.")
        dead = ("Pure single-bit-flip SA from random starts plateaued around 750-900; only "
                "restarting from the 578 record and using large perturbations found lower energies. "
                "Most runs still returned 578, so 574 is a narrow target.")
    elif pid == "p009-job-shop-ta41":
        orders = [list(map(int, line.split())) for line in open(cand) if line.strip()]
        assert len(orders) == 20 and all(len(r) == 30 for r in orders), "p009 candidate must be 20x30"
        cert = {"machines": orders}
        approach = ("Tabu search on the disjunctive graph using the critical-path neighbourhood "
                    "(swap a critical operation with its machine neighbour), with a tabu list, "
                    "aspiration criterion and periodic diversification kicks.")
        dead = ("Random machine permutations create cycles and are invalid; search must start from a "
                "valid schedule. Escaping the 2006 basin requires long runs; many restarts stalled at 2006.")
    else:
        print("unknown problem id", pid); sys.exit(2)

    (shot_dir / "certificate.json").write_text(json.dumps(cert, ensure_ascii=False))
    shot_md = f"""+++
login = "{login}"
builds_on = []
claimed_score = "PLACEHOLDER"
agent = "{AGENT}"
model = "{MODEL}"
+++

## 思路

{approach}

## 哪里走不通

{dead}
"""
    (shot_dir / "SHOT.md").write_text(shot_md)
    # run the verifier to get the real score, then fill claimed_score
    vout = subprocess.run(["python3", str(pdir/"verify/verify.py"), str(shot_dir)],
                          capture_output=True, text=True).stdout.strip()
    score = json.loads(vout)["score"]
    shot_md = shot_md.replace('claimed_score = "PLACEHOLDER"', f'claimed_score = "{score}"')
    (shot_dir / "SHOT.md").write_text(shot_md)
    print(f"verifier score = {score}")
    print("running tools/check.py ...")
    r = subprocess.run(["python3", str(REPO/"tools/check.py"), str(shot_dir)], cwd=REPO)
    sys.exit(r.returncode)

if __name__ == "__main__":
    main()
