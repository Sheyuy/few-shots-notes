#!/bin/bash
# cycle.sh -- one round of the p003 self-feeding loop.
#
#   1. merge every candidate produced by the hunters into the base pool
#      (merge_new35.py verifies each one with two independent implementations)
#   2. push the enlarged pool to the notes repo
#   3. dispatch a fresh closure run and a fresh hunt on the enlarged pool
#
# Usage:  ./cycle.sh <candidate-dir-or-glob> [...]
#
# The candidate arguments are glob patterns for NEW35_*.json files.
set -e
W=/workspace/few-shots-work
P=$W/p003-ramsey-r46
cd "$P"

CAND=()
for pat in "$@"; do
  for f in $pat; do
    [ -f "$f" ] && CAND+=("$f")
  done
done

if [ ${#CAND[@]} -gt 0 ]; then
  echo "== merging ${#CAND[@]} candidate(s) =="
  python3 merge_new35.py pool35_extended.json "${CAND[@]}"
else
  echo "== no candidates to merge =="
fi
N=$(python3 -c "import json;print(len(json.load(open('pool35_extended.json'))))")
echo "== pool size: $N =="

echo "== pushing the pool to the notes repo =="
python3 - <<'PY'
import json
W = "/workspace/few-shots-work"
json.dump({"src/mavis/p003/pool35_extended.json": f"{W}/p003-ramsey-r46/pool35_extended.json"},
          open(f"{W}/mf_cycle.json", "w"), indent=1)
PY
python3 "$W/push_notes.py" mf_cycle.json "mavis: p003 base pool update from the self-feeding loop" | tail -1

echo "== dispatching closure + hunt on the enlarged pool =="
python3 - <<'PY'
import json, time, urllib.request, urllib.error, random
PAT = open('/workspace/.fs_pat').read().strip()
API = "https://api.github.com/repos/Sheyuy/few-shots-notes"

def post(wf, inp):
    r = urllib.request.Request(f"{API}/actions/workflows/{wf}/dispatches",
                               data=json.dumps({"ref": "main", "inputs": inp}).encode(),
                               method="POST")
    r.add_header("Authorization", "Bearer " + PAT)
    r.add_header("Accept", "application/vnd.github+json")
    try:
        with urllib.request.urlopen(r, timeout=25) as resp:
            return resp.status
    except urllib.error.HTTPError as e:
        return "HTTP %d" % e.code

seed = random.randrange(10000)
print("  p003-closure-sat:", post("p003-closure-sat.yml", {"rounds": "6", "minutes": "280"}))
time.sleep(1)
print("  p003-hunt seed=%d:" % seed, post("p003-hunt.yml", {"trials": "4000", "secs": "3", "seed": str(seed)}))
PY
echo "== cycle done =="
