#!/bin/bash
cd /Users/shiny/.sclaw/workspace/few-shots-work/p004
IN=/Users/shiny/.sclaw/workspace/few-shots-notes/data/scnet/p004/cc_10_7_134753_paper.txt
for i in 0 1 2 3 4 5 6 7; do
  ./sixm3_shard --input "$IN" --max-k 6 --only-deep3 --shard $i/256 --threads 1 --seconds 300 \
    >> shard_r3_slice.log 2>&1
  echo "DONE shard $i/256" >> shard_r3_slice.log
done
