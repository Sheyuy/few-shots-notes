"""Check a one-point-per-line file with the official p004 verifier.

    python3.11+ verify_points.py cc_10_7_134753_paper.txt

Needs Python 3.11+ because the official verifier imports ``tomllib``.
Set FEW_SHOTS_REPO if the few-shots clone is not at the default path.
"""
import json
import os
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(os.environ.get('FEW_SHOTS_REPO', '/Users/shiny/.sclaw/workspace/few-shots'))
VERIFIER = REPO / 'problems' / 'p004-shannon-capacity-c7' / 'verify' / 'verify.py'

if len(sys.argv) != 2:
    raise SystemExit('usage: python3 verify_points.py <points.txt>')

pts = [''.join(line.split()) for line in Path(sys.argv[1]).read_text().splitlines() if line.strip()]
with tempfile.TemporaryDirectory() as d:
    Path(d, 'certificate.json').write_text(json.dumps({'points': pts}))
    r = subprocess.run([sys.executable, str(VERIFIER), d], capture_output=True, text=True)
if r.returncode != 0 and 'tomllib' in r.stderr:
    raise SystemExit('need Python 3.11+ (the official verifier imports tomllib); '
                     'run this with python3.12 or python3.13')
print(r.stdout.strip() or r.stderr.strip())
