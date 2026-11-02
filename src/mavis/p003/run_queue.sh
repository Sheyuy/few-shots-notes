#!/bin/bash
# run_queue.sh -- two-slot supervisor for the 36-vertex (4,6) SAT queue.
#
#   ./run_queue.sh
#
# Slot A: mode-4 (symmetry break + verified degree constraints) for d = 11, 12, 15
# Slot B: mode-5 (NO symmetry break + degree constraints, covers every d)
#         then mode-4 for d = 13, 16, 17
#
# Why this shape:
#   * the d-instances partition the solution space by "does the graph have a
#     vertex of degree d", so running all of them separately is expensive;
#     mode-5 has no symmetry break and therefore covers every solution at once
#     -> it is the safety net;
#   * mode-4 additionally prunes with the (verified) degree constraint and
#     breaks the S_d x S_(35-d) symmetry, so it is the faster per-d search;
#   * both are run, so neither bet is fatal.
#
# Robustness: the outer `while true` restarts the supervisor itself, each
# instance writes its own log, an instance that already has a verdict is
# skipped, and the supervisor waits for any kissat already working on the
# same CNF instead of starting a second one.

cd /workspace/few-shots-work/p003-ramsey-r46 || exit 1
VERD="sat36_verdicts.txt"
TIMEOUT="${TIMEOUT:-7200}"

wait_for_cnf() {           # wait until no kissat is working on $1
    while pgrep -f "kissat $1" >/dev/null 2>&1; do sleep 20; done
}

run_list() {               # $1 = tag, rest = "cnf:tag" items
    local tag="$1"; shift
    for item in $@; do
        local cnf="${item%%:*}" tagid="${item##*:}"
        local LOG="q_${tagid}.log"
        wait_for_cnf "$cnf"
        if [ -f "$LOG" ] && grep -q "^s " "$LOG"; then continue; fi
        echo "=== $tagid START $(date -Is) ===" >> "$LOG"
        timeout "$TIMEOUT" ./kissat "$cnf" >> "$LOG" 2>&1
        local rc=$?
        echo "=== $tagid EXIT rc=$rc $(date -Is) ===" >> "$LOG"
        grep -E "^s " "$LOG" | tail -1 >> "$LOG"
        echo "$tagid $(grep -E '^s ' "$LOG" | tail -1) rc=$rc" >> "$VERD"
        if grep -q "^s SATISFIABLE" "$LOG"; then
            echo "=== SOLUTION FOUND on $tagid ===" >> "$LOG"
            return 0
        fi
    done
    return 1
}

while true; do
    # Slot A -- waits for the still-running mode-2 d=11 experiment first
    ( wait_for_cnf cnf36/s36_d11.cnf
      run_list A cnf36/s36m4_d11.cnf:m4d11 cnf36/s36m4_d12.cnf:m4d12 cnf36/s36m4_d15.cnf:m4d15 ) &
    A=$!
    # Slot B -- waits for the still-running mode-4 d=14 experiment first
    ( wait_for_cnf cnf36/s36m4_d14.cnf
      run_list B cnf36/s36m5_plain.cnf:m5plain cnf36/s36m4_d13.cnf:m4d13 \
                   cnf36/s36m4_d16.cnf:m4d16 cnf36/s36m4_d17.cnf:m4d17 ) &
    B=$!
    wait $A $B
    if grep -l "^s SATISFIABLE" q_*.log >/dev/null 2>&1; then break; fi
    sleep 5
done
