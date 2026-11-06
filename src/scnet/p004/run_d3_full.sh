#!/bin/bash
cd /Users/shiny/.sclaw/workspace/few-shots-work/p004
IN=/Users/shiny/.sclaw/workspace/few-shots-notes/data/scnet/p004/cc_10_7_134753_paper.txt
LOG=/Users/shiny/.sclaw/workspace/few-shots-notes/logs/scnet/p004/sixm3_deep3_k6_all64.log
for i in $(seq 0 63); do
  /tmp/sixm3_d3 --input "$IN" --max-k 6 --only-deep3 --shard $i/64 --threads 8 --seconds 600 >> "$LOG" 2>&1
  echo "DONE shard $i/64 rc=$?" >> "$LOG"
done
echo ALL_DONE >> "$LOG"
