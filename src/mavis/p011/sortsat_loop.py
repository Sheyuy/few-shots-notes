#!/usr/bin/env python3
"""sortsat_loop.py -- the "iterative encoding" driver for sorting-network SAT.

Protocol (Codish et al. 2015, arXiv:1507.01428):
  1. encode "exists a sorting network with <= maxC comparators on n channels
     with d layers, respecting the fixed prefix" for the CURRENT set of 0/1
     test inputs;
  2. solve with kissat;
     - UNSAT  -> no such network exists for this prefix and size  => done;
     - SAT    -> extract the network;
  3. check the network against ALL 2^n inputs with the independent bit-sliced
     verifier (netcheck.c);
     - sorts everything -> a real network, report it (this would break the
       world record for n = 17);
     - otherwise -> add one counterexample input to the test set and go back
     to step 1.

Usage:
  sortsat_loop.py <n> <d> <maxC> <prefix.txt> <workdir> [minutes] [kissat]
"""
import os
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
SORTNET = os.path.join(HERE, "sortsat")
NETCHECK = os.path.join(HERE, "netcheck")


def pidx(i, j, n):
    return i * (2 * n - i - 1) // 2 + (j - i - 1)


def seed_inputs(n):
    """A small but well-spread set of 0/1 inputs."""
    s = set()
    for k in range(n):
        s.add(1 << k)
        s.add(((1 << n) - 1) ^ (1 << k))
    s.add(0)
    s.add((1 << n) - 1)
    alt = 0
    for i in range(0, n, 2):
        alt |= 1 << i
    s.add(alt)
    s.add(((1 << n) - 1) ^ alt)
    return sorted(s)


def extract(model_file, n, d):
    NP = n * (n - 1) // 2
    base_g = 1
    true = set()
    with open(model_file) as f:
        for line in f:
            if not line.startswith('v '):
                continue
            for tok in line.split()[1:]:
                t = int(tok)
                if t > 0:
                    true.add(t)
    comps = []
    for k in range(d):
        for i in range(n):
            for j in range(i + 1, n):
                if base_g + k * NP + pidx(i, j, n) in true:
                    comps.append((i, j))
    return comps


def main():
    n = int(sys.argv[1]); d = int(sys.argv[2]); maxC = int(sys.argv[3])
    prefix = sys.argv[4]; work = sys.argv[5]
    minutes = float(sys.argv[6]) if len(sys.argv) > 6 else 300.0
    kissat = sys.argv[7] if len(sys.argv) > 7 else "kissat"
    os.makedirs(work, exist_ok=True)
    inp_file = os.path.join(work, "inputs.txt")
    if not os.path.exists(inp_file):
        with open(inp_file, 'w') as f:
            f.write('\n'.join(map(str, seed_inputs(n))) + '\n')
    deadline = time.time() + minutes * 60
    it = 0
    while time.time() < deadline:
        it += 1
        cnf = os.path.join(work, "cur.cnf")
        log = os.path.join(work, "cur.log")
        r = subprocess.run([SORTNET, str(n), str(d), str(maxC), prefix, inp_file, cnf],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print("sortsat failed:", r.stderr[-400:], flush=True)
            time.sleep(5)
            continue
        print("[it %d] inputs=%d" % (it, len(open(inp_file).read().split())), flush=True)
        with open(log, 'w') as lf:
            p = subprocess.run([kissat, cnf], stdout=lf, stderr=subprocess.STDOUT)
        txt = open(log).read()
        if 's UNSATISFIABLE' in txt:
            open(os.path.join(work, "RESULT_UNSAT"), 'w').write("unsat after %d iterations\n" % it)
            print("UNSAT: no network with <=%d comparators for this prefix" % maxC, flush=True)
            return
        if 's SATISFIABLE' not in txt:
            print("solver did not finish; continuing", flush=True)
            continue
        # extract and verify independently
        comps = extract(log, n, d)
        cf = os.path.join(work, "cand.txt")
        with open(cf, 'w') as f:
            for (a, b) in comps:
                f.write("%d %d\n" % (a, b))
        v = subprocess.run([NETCHECK, cf, str(n)], capture_output=True, text=True)
        print("[it %d] candidate: %d comparators | %s" % (it, len(comps), v.stdout.strip()), flush=True)
        if v.returncode == 0:
            import json
            net = {"comparators": [[a, b] for (a, b) in comps]}
            json.dump(net, open(os.path.join(work, "NETWORK.json"), 'w'))
            open(os.path.join(work, "RESULT_FOUND"), 'w').write(
                "%d comparators, sorts all %d inputs\n" % (len(comps), 1 << n))
            print("*** FOUND A SORTING NETWORK WITH %d COMPARATORS ***" % len(comps), flush=True)
            return
        # add the counterexample input and iterate
        ce = None
        for t in v.stdout.split():
            if t.startswith('first_counterexample='):
                ce = int(t.split('=')[1])
                break
        if ce is None or ce < 0:
            print("no counterexample found; stopping", flush=True)
            return
        cur = set(int(x) for x in open(inp_file).read().split())
        if ce in cur:
            print("counterexample %d already in the test set -- stuck" % ce, flush=True)
            open(os.path.join(work, "RESULT_STUCK"), 'w').write("ce %d already present" % ce)
            return
        cur.add(ce)
        with open(inp_file, 'w') as f:
            f.write('\n'.join(map(str, sorted(cur))) + '\n')
        print("[it %d] added counterexample %d" % (it, ce), flush=True)
    open(os.path.join(work, "RESULT_TIMEOUT"), 'w').write("timeout after %d iterations\n" % it)
    print("time limit reached", flush=True)


if __name__ == '__main__':
    main()
