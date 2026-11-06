import json, sys, time
sys.path.insert(0, '/Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004')
import os
os.environ.setdefault('FEW_SHOTS_REPO', '/Users/shiny/.sclaw/workspace/few-shots')
import sweep

OLD = tuple(int(x) for x in sys.argv[1].split(','))
NEW = tuple(int(x) for x in sys.argv[2].split(','))
rounds = int(sys.argv[3]) if len(sys.argv) > 3 else 4
secs = float(sys.argv[4]) if len(sys.argv) > 4 else 150.0
OUT = '/Users/shiny/.sclaw/workspace/few-shots-work/p004/asym_variant_ckpt.json'

ck = None
for r in range(rounds):
    sweep.ASYM_CHECKPOINT = ck
    t0 = time.time()
    ck = sweep.asymmetric_patch_search(seconds=secs, max_new=150, cp_seconds=2.0,
                                       max_expansions=3000, rep_old=OLD, rep_new=NEW,
                                       root_holes=None)
    with open(OUT, 'w') as f:
        json.dump(ck, f)
    h = ck['history']
    holes = [x['holes'] for x in h if x['holes'] is not None]
    lbs = [x['lower_bound'] for x in h if x['lower_bound'] is not None]
    print('ROUND', r + 1, 'classes', len(h), 'min_holes', min(holes) if holes else None,
          'min_lb', min(lbs) if lbs else None, 'pending', len(ck['frontier']),
          'statuses', ck['last_summary']['statuses'], flush=True)
    if ck['witness']:
        print('WITNESS_FOUND', flush=True)
        break
    if not ck['frontier']:
        print('FRONTIER_EMPTY', flush=True)
        break
