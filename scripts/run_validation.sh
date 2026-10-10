#!/bin/sh
# 既知の Seal 型の族（2・4・11・26進）を自動で再発見し、9進の族も見つけるまでを再現する（数分）
set -e
# timeout が無い環境（macOS 標準など）では制限なしで実行
if command -v timeout >/dev/null 2>&1; then TO="timeout"; elif command -v gtimeout >/dev/null 2>&1; then TO="gtimeout"; else TO=""; fi
lim() { if [ -n "$TO" ]; then $TO "$@"; else shift; "$@"; fi; }
cd "$(dirname "$0")/.."
mkdir -p work/validation && cd work/validation
for BP in "2 4" "4 6" "26 4" "11 6" "9 10"; do
  set -- $BP
  echo "=== base $1, period $2 ==="
  ../../bin/outauto_s Aa $1 $2 0 > outer_B$1_P$2.log
  for f in outer_B$1_P$2_*.txt; do
    [ -f "$f" ] || continue
    ov=$(head -1 "$f" | awk '{print $4}'); cin=$(head -1 "$f" | awk '{print $5}')
    echo "-- $f"
    lim 600 ../../bin/coreauto2 $1 $ov $cin "$f" | grep -E "par0|simulation" | cut -c1-160
  done
done
