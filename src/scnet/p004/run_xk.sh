#!/bin/bash
cd /Users/shiny/.sclaw/workspace/few-shots-work/p004
echo "# xk: complete k-exchange check for the 367-point set R in C7^5 (2026-11-01)" 
echo "# usage: ./xk K   (removes K points of R, asks for K+1 outside points, |T(A)|<=K)"
for k in 3 4 5 6; do
  echo "== K=$k"
  ./xk2 $k 20000000000 2>/dev/null
done
