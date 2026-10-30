#!/bin/bash
# 部署到远端 /workspace/p005/ 后执行；前置：PYTHONPATH=/workspace/pylibs 下的 ortools==9.11.4210
set -u
cd /workspace/p005
mkdir -p logs results
export PYTHONPATH=/workspace/pylibs
PY=/usr/bin/python3.10

# 变体 A：固定 0,e1..e8 的对称破缺 + 最大化
setsid env PYTHONPATH=/workspace/pylibs nohup $PY /workspace/p005/symbreak.py \
  8 21600 64 fix /workspace/p005/results/sym8_max.json \
  > /workspace/p005/logs/sym8_max.log 2>&1 &

# 变体 B：固定 0,e1..e8 + 可行性约束 sum >= 513
setsid env PYTHONPATH=/workspace/pylibs nohup $PY /workspace/p005/symbreak.py \
  8 21600 64 fix /workspace/p005/results/sym8_feas.json 513 \
  > /workspace/p005/logs/sym8_feas.log 2>&1 &

# 变体 C：对照（不做标准点破缺，仅最大���），用于确认破缺是否真的帮助
setsid env PYTHONPATH=/workspace/pylibs nohup $PY /workspace/p005/symbreak.py \
  8 21600 32 nofix /workspace/p005/results/sym8_nofix.json \
  > /workspace/p005/logs/sym8_nofix.log 2>&1 &

echo "launched: sym8_max / sym8_feas / sym8_nofix"