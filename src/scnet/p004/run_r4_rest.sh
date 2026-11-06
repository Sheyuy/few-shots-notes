#!/bin/bash
cd /Users/shiny/.sclaw/workspace/few-shots-work/p004
IN=/Users/shiny/.sclaw/workspace/few-shots-notes/data/scnet/p004/cc_10_7_134753_paper.txt
for i in $(seq 16 63); do
  ./sixm3_shard --input "$IN" --max-k 6 --only-deep6 --shard $i/64 --threads 1 --seconds 300 \
    >> shard_r4_rest.log 2>&1
  echo "DONE shard $i/64" >> shard_r4_rest.log
done
echo ALL_DONE >> shard_r4_rest.log
