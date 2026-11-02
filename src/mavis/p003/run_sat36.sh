#!/bin/bash
# Supervisor: run the 36-vertex (4,6) SAT instances under kissat.
# $1 = space separated list of d values, $2 = tag
cd /workspace/few-shots-work/p003-ramsey-r46
LOG=sat36_$2.log
for d in $1; do
  if grep -q "d=$d .*DONE" $LOG 2>/dev/null; then echo "d=$d already decided, skip" >> $LOG; continue; fi
  echo "=== d=$d START $(date -Is) ===" >> $LOG
  timeout 7200 ./kissat cnf36/s36_d$d.cnf >> $LOG 2>&1
  rc=$?
  echo "=== d=$d EXIT rc=$rc $(date -Is) ===" >> $LOG
  grep -E "^s " $LOG | tail -1 >> $LOG
done
