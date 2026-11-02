#!/bin/bash
# 远端执行：固定层 0 的 171 点子集，优化层 1+2（目标总 513）
# 前置：/workspace/p005/{fixed_layer21.py,layers.py}、/workspace/p005/baseline.json、PYTHONPATH 下 ortools
set -u
cd /workspace/p005
mkdir -p logs results
export PYTHONPATH=/workspace/pylibs
PY=/usr/bin/python3.10
SEEDS="${1:-1 2 3 4}"
for s in $SEEDS; do
  setsid env PYTHONPATH=/workspace/pylibs nohup $PY /workspace/p005/fixed_layer21.py \
    171 "$s" 1800 32 > /workspace/p005/logs/layer21_$s.log 2>&1 &
done
echo "launched seeds: $SEEDS"