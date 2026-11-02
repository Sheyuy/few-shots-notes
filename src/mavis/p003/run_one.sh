#!/bin/bash
# run_one.sh <cnf> <log> <timeout>
cd /workspace/few-shots-work/p003-ramsey-r46 || exit 1
CNF="$1"; LOG="$2"; T="${3:-7200}"
echo "=== START $(date -Is) ===" >> "$LOG"
timeout "$T" ./kissat "$CNF" >> "$LOG" 2>&1
rc=$?
echo "=== EXIT rc=$rc $(date -Is) ===" >> "$LOG"
grep -E "^s " "$LOG" | tail -1 >> "$LOG"
