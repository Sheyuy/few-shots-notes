import json, os, sys, time
sys.path.insert(0, '/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004')
os.environ.setdefault('FEW_SHOTS_REPO', '/Users/shiny/.sclaw/workspace/few-shots')
import sweep

OUT = '/Users/shiny/.sclaw/workspace/few-shots-work/p004/asym_ckpt_latest.json'
rounds = int(sys.argv[1]) if len(sys.argv) > 1 else 4
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 170.0

for r in range(rounds):
    ck = sweep.asymmetric_patch_search(seconds=secs, max_new=150, cp_seconds=2.0, max_expansions=3000)
    sweep.ASYM_CHECKPOINT = ck
    with open(OUT, 'w') as f:
        json.dump(ck, f)
    s = ck['last_summary']
    keep = {k: s[k] for k in ('new_models', 'statuses', 'expanded_states_total', 'pending_states',
                              'smallest_feasible_omissions', 'unresolved_target_classes',
                              'verified_improvement')}
    print('ROUND', r + 1, json.dumps(keep), flush=True)
    if ck['witness']:
        print('WITNESS_FOUND', flush=True)
        break
    if not ck['frontier']:
        print('FRONTIER_EMPTY', flush=True)
        break
