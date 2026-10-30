import json, sys, os, subprocess
src = sys.argv[1]
rows = [l.strip() for l in open(src) if l.strip()]
n = len(rows)
assert all(len(r) == n for r in rows), "row length mismatch"
for i in range(n):
    assert rows[i][i] == '0', "diagonal"
    for j in range(n):
        assert rows[i][j] == rows[j][i], "not symmetric"
os.makedirs("cand036", exist_ok=True)
json.dump({"adjacency": rows}, open("cand036/certificate.json", "w"))
print("n =", n)
r = subprocess.run(["python3", "/workspace/few-shots/problems/p003-ramsey-r46/verify/verify.py",
                    "/workspace/few-shots-work/p003/cand036"], capture_output=True, text=True)
print(r.stdout.strip())
print(r.stderr.strip())
print("exit", r.returncode)
