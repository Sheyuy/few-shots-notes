#!/bin/bash
set -o pipefail
if [ "$#" -lt 4 ] || [ "$#" -gt 5 ]; then
  echo "usage: run_compressed_sat.sh model.cnf.gz log rcfile seed [kissat_path]" >&2
  exit 2
fi
MODEL=$1
LOG=$2
RCFILE=$3
SEED=$4
KISSAT=${5:-${KISSAT:-kissat}}
SOLVER=("$KISSAT")
if [ -n "${LOADER:-}" ]; then
  SOLVER=("$LOADER" "$KISSAT")
fi
rm -f "$RCFILE"
gzip -dc "$MODEL" | "${SOLVER[@]}" --time="${TIME:-1200}" --seed="$SEED" --sat >"$LOG" 2>&1
rc=$?
printf '%s\n' "$rc" >"$RCFILE"
exit "$rc"
