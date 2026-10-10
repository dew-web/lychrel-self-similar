#!/bin/sh
# 10進・0/9 の2列型: 周期 $1..$2 の全桁あふれパターンで端の解を調べ、見つかれば中心を判定する
#   例: sh scripts/classify_base10.sh 2 15   （周期16以上は時間と記憶容量が大きくなる）
set -e
# timeout が無い環境（macOS 標準など）では制限なしで実行
if command -v timeout >/dev/null 2>&1; then TO="timeout"; elif command -v gtimeout >/dev/null 2>&1; then TO="gtimeout"; else TO=""; fi
lim() { if [ -n "$TO" ]; then $TO "$@"; else shift; "$@"; fi; }
cd "$(dirname "$0")/.."
mkdir -p work/base10 && cd work/base10
for P in $(seq ${1:-2} ${2:-15}); do
  ../../bin/outauto_s Aa 10 $P 0 > A10_P$P.txt
  echo "P=$P patterns=$(grep -c 'ov=' A10_P$P.txt) with_outer=$(grep -c 'terminals=[1-9]' A10_P$P.txt) over_limit=$(grep -c CAPACITY A10_P$P.txt)"
  for f in outer_B10_P${P}_*.txt; do
    [ -f "$f" ] || continue
    ov=$(head -1 "$f" | awk '{print $4}'); cin=$(head -1 "$f" | awk '{print $5}')
    lim 1800 ../../bin/coreauto2 10 $ov $cin "$f" | grep -E "par0" | cut -c1-160
  done
done
