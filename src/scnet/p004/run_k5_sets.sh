#!/bin/bash
cd /Users/shiny/.sclaw/workspace/few-shots-notes/src/scnet/p004
LOG=/Users/shiny/.sclaw/workspace/few-shots-notes/logs/scnet/p004/sixm3_k5_five_sets_2026-11-02.log
for f in cc_10_7_134753_9pair17 cc_10_7_134753_r1c19 cc_10_7_134753_r2c3; do
  echo "===== SET $f" >> "$LOG"
  /usr/bin/time -p /tmp/sixm3_d3 --input ../../../data/scnet/p004/$f.txt --max-k 5 --threads 8 --seconds 600 >> "$LOG" 2>&1
  echo "DONE $f rc=$?" >> "$LOG"
done
echo ALL_DONE >> "$LOG"
