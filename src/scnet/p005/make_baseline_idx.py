"""Write the p005 baseline as integer indices (one per line) on stdout.

The C search programs (rk13, capsearch2, deep, oneswap, grow2) read a file named
baseline.idx from the working directory.

    python3 make_baseline_idx.py > baseline.idx
"""
import os
import sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
from paths import P005
import json


def idx(p):
    i = 0
    for d in p:
        i = 3 * i + d
    return i


cert = json.load(open(str(P005 / 'baseline' / 'certificate.json')))
print('\n'.join(str(idx(p)) for p in cert['points']))
