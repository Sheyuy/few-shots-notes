#!/usr/bin/env python3
"""Convert a plain orders text file (20 lines x 30 job numbers, space separated, 0..29)
into the certificate.json format the official verifier expects:
    {"machines": [[30 jobs] x 20]}  (jobs 0..29)"""
import json, sys

def main():
    if len(sys.argv) < 2:
        print("usage: to_cert.py <orders.txt> [out.json]")
        return 2
    infile = sys.argv[1]
    outfile = sys.argv[2] if len(sys.argv) > 2 else "certificate.json"
    machines = []
    with open(infile) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            nums = [int(x) for x in line.split()]
            machines.append(nums)
    assert len(machines) == 20, f"expected 20 machines, got {len(machines)}"
    for i, m in enumerate(machines):
        assert len(m) == 30, f"machine {i} expected 30 jobs, got {len(m)}"
        assert sorted(m) == list(range(30)), f"machine {i} is not a permutation of 0..29: {sorted(m)}"
    cert = {"machines": machines}
    with open(outfile, "w") as f:
        json.dump(cert, f)
    print(f"wrote {outfile}: {len(machines)} machines x {len(machines[0])} jobs")
    return 0

if __name__ == "__main__":
    sys.exit(main())
